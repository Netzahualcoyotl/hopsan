//$Id$

#include "RemoteHopsanClient.h"

#include "msgpack.hpp"
#include "../include/Messages.h"
#include "../include/MessageUtilities.h"

#include <iostream>
#include <thread>
#include <algorithm>

using namespace std;
const int gLinger_ms = 1000;

// ---------- Help functions start ----------

typedef std::chrono::duration<double> fseconds;
inline double elapsedSecondsSince(chrono::steady_clock::time_point &rSince)
{
    return chrono::duration_cast<fseconds>(chrono::steady_clock::now()-rSince).count();
}

template <typename T>
void sendClientMessage(zmq::socket_t *pSocket, MessageIdsEnumT id, const T &rMessage)
{
    try
    {
        sendMessage(*pSocket, id, rMessage);
        //pSocket->send(static_cast<void*>(out_buffer.data()), out_buffer.size());
    }
    catch(zmq::error_t e)
    {
        //Ignore
    }
}


void sendShortClientMessage(zmq::socket_t *pSocket, MessageIdsEnumT id)
{
    try
    {
        sendShortMessage(*pSocket, id);
    }
    catch(zmq::error_t e)
    {
        //Ignore
    }
}

bool receiveAckNackMessage(zmq::socket_t *pSocket, long timeout, string &rNackReason)
{
    zmq::message_t response;
    if(receiveWithTimeout(*pSocket, timeout, response))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        //cout << "id: " << id << endl;
        if (id == Ack)
        {
            return true;
        }
        else if (id == NotAck)
        {
            rNackReason = unpackMessage<std::string>(response, offset, parseOK);
        }
        else
        {
            rNackReason = "Got neither Ack nor Nack";
        }
    }
    else
    {
        rNackReason = "Got either timeout or exception in zmq::recv()";
    }
    return false;
}

bool receiveAckNackMessage(zmq::socket_t *pSocket, long timeout)
{
    string dummy;
    return receiveAckNackMessage(pSocket, timeout, dummy);
}

// ---------- Help functions end ----------

bool RemoteHopsanClient::connectToServer(std::string address)
{
    if (serverConnected())
    {
        setLastError("You are already connected to a server");
        return false;
    }
    else
    {
        try
        {
            mpServerSocket = new zmq::socket_t(*mpContext, ZMQ_REQ);
            mpServerSocket->setsockopt(ZMQ_LINGER, &gLinger_ms, sizeof(int));

            std::string ip,port,fullrelayid,baserelayid,subrelayid;
            splitaddressandrelayid(address,ip,port,fullrelayid,baserelayid,subrelayid);

            if(!baserelayid.empty() && subrelayid.empty() && addressServerConnected())
            {
                requestRelaySlot(baserelayid,-1,fullrelayid);
            }

            mServerRelayIdentity = fullrelayid;
            mServerAddress = makeZMQAddress(ip,port);

            if (!mServerRelayIdentity.empty())
            {
                mpServerSocket->setsockopt( ZMQ_IDENTITY, (void*)mServerRelayIdentity.data(), mServerRelayIdentity.size());
                cout << " Set server relay socket identity: " << mServerRelayIdentity << endl;
            }
            mpServerSocket->connect(mServerAddress.c_str());
            return true;
        }
        catch (zmq::error_t e)
        {
            setLastError(e.what());
            deleteSockets();
        }
        return false;
    }
}

bool RemoteHopsanClient::serverConnected() const
{
    // Note we can not use socket.connected() it only checks if underlying c-socket exist
    return mpServerSocket && !mServerAddress.empty();
}


bool RemoteHopsanClient::sendGetParamMessage(const string &rName, string &rValue)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    reqmsg_RequestParameter_t msg {rName};
    sendClientMessage(mpWorkerSocket, RequestParameter, msg);

    zmq::message_t response;
    if (receiveWithTimeout(*mpWorkerSocket, response, mShortReceiveTimeout))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyParameter)
        {
            rValue = unpackMessage<string>(response, offset, parseOK);
            assert(response.size() == offset);
            return true;
        }
        else
        {
            rValue.clear();
            return false;
        }
    }
    return false;
}

bool RemoteHopsanClient::sendSetParamMessage(const string &rName, const string &rValue)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    cmdmsg_SetParameter_t msg {rName, rValue};
    sendClientMessage(mpWorkerSocket, SetParameter, msg);

    string err;
    bool rc = receiveAckNackMessage(mpWorkerSocket, mShortReceiveTimeout, err);
    if (!rc)
    {
        cout << err << endl;
    }
    return rc;
}

bool RemoteHopsanClient::sendModelMessage(const std::string &rModel)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendClientMessage<std::string>(mpWorkerSocket, SetModel, rModel);
    string err;
    bool rc = receiveAckNackMessage(mpWorkerSocket, mShortReceiveTimeout, err);
    if (!rc)
    {
        cout << err << endl;
    }
    return rc;
}

bool RemoteHopsanClient::sendSimulateMessage(const int nLogsamples, const int logStartTime,
                         const int simStarttime, const int simSteptime, const int simStoptime)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    cmdmsg_Simulate_t msg;// {nLogsamples, logStartTime, simStarttime, simSteptime, simStoptime};
    sendClientMessage(mpWorkerSocket, Simulate, msg);
    string err;
    bool rc = receiveAckNackMessage(mpWorkerSocket, mShortReceiveTimeout, err);
    if (!rc)
    {
        cout << err << endl;
    }
    return rc;
}

bool RemoteHopsanClient::abortSimulation()
{
    // Lock here to prevent problem if some other thread is requesting fore xample status
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendShortClientMessage(mpWorkerSocket, Abort);
    string err;
    bool rc = receiveAckNackMessage(mpWorkerSocket, mShortReceiveTimeout, err);
    if (!rc)
    {
        cout << err << endl;
    }
    return rc;
}

bool RemoteHopsanClient::blockingBenchmark(const string &rModel, const int nThreads, double &rSimTime)
{
    bool gotResponse=false;
    int ctrlPort;
    bool rc = requestSlot(nThreads, ctrlPort);
    if(rc)
    {
        rc = connectToWorker(ctrlPort);
    }
    else
    {
        cout << mLastErrorMessage << endl;
    }

    if (rc)
    {
        cmdmsg_Benchmark_t msg;
        msg.model = rModel;
        sendClientMessage(mpWorkerSocket, Benchmark, msg);
        string errorMsg;
        rc = receiveAckNackMessage(mpWorkerSocket, 5000,  errorMsg);

        if (rc)
        {
            // Block until benchamark is done
            //! @todo what if benchmark freeces, need a timeout here
            double progress; bool isAlive;
            std::thread t(&RemoteHopsanClient::requestWorkerStatusThread, this, &progress, &isAlive);
            t.join();

            // Now request benchmark results
            gotResponse = requestBenchmarkResults(rSimTime);
        }
        else
        {
            cout << errorMsg << endl;
        }
    }

    disconnectWorker();

    return gotResponse;
}

bool RemoteHopsanClient::requestBenchmarkResults(double &rSimTime)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendShortClientMessage(mpWorkerSocket, RequestBenchmarkResults);
    zmq::message_t reply;
    if (receiveWithTimeout(*mpWorkerSocket, reply, mShortReceiveTimeout))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(reply, offset, parseOK);
        if (id == ReplyBenchmarkResults)
        {
            replymsg_ReplyBenchmarkResults_t results = unpackMessage<replymsg_ReplyBenchmarkResults_t>(reply, offset, parseOK);
            if (parseOK)
            {
                //! @todo right now we bundle all times togheter
                rSimTime = results.inittime+results.simutime+results.finitime;
                return true;
            }
        }
    }
    return false;
}

bool RemoteHopsanClient::requestWorkerStatus(WorkerStatusT &rWorkerStatus)
{
    // Lock here to prevent problem if som other thread is requesting abort
    std::lock_guard<std::mutex> lock(mWorkerMutex); //! @todo should use this mutex in more places (or build it into the send function)

    sendShortClientMessage(mpWorkerSocket, RequestWorkerStatus);

    zmq::message_t response;
    if (receiveWithTimeout(*mpWorkerSocket, response, mShortReceiveTimeout))
    {
        //cout << "Response size: " << response.size() << endl;
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyWorkerStatus)
        {
            replymsg_ReplyWorkerStatus_t status = unpackMessage<replymsg_ReplyWorkerStatus_t>(response, offset, parseOK);
            if (parseOK)
            {
                rWorkerStatus = status;
                return true;
            }
        }
    }
    return false;
}

bool RemoteHopsanClient::requestServerStatus(ServerStatusT &rServerStatus)
{
    sendShortClientMessage(mpServerSocket, RequestServerStatus);

    zmq::message_t response;
    if (receiveWithTimeout(*mpServerSocket, response, mShortReceiveTimeout))
    {
        //cout << "Response size: " << response.size() << endl;
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyServerStatus)
        {
            replymsg_ReplyServerStatus_t status = unpackMessage<replymsg_ReplyServerStatus_t>(response, offset, parseOK);
            if (parseOK)
            {
                rServerStatus = status;
                //cout << "Got status reply" << endl;
                return true;
            }
            cout << "PArse error" << endl;
        }
        else if (id == NotAck)
        {
            std::string err = unpackMessage<std::string>(response, offset, parseOK);
            cout << "Received NotAck: " << err << endl;
        }
    }
    return false;
}

bool RemoteHopsanClient::requestSimulationResults(vector<string> *pDataNames, vector<double> *pData)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendClientMessage<string>(mpWorkerSocket, RequestResults, "*"); // Request all

    zmq::message_t response;
    if (receiveWithTimeout(*mpWorkerSocket, response, mLongReceiveTimeout))
    {
        //cout << "Response size: " << response.size() << endl;
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyResults)
        {
            vector<replymsg_ResultsVariable_t> vars = unpackMessage<vector<replymsg_ResultsVariable_t>>(response,offset, parseOK);
            //cout << "Received: " << vars.size() << " vars" << endl;
            size_t nLogSamples;
            if (!vars.empty())
            {
                nLogSamples = vars[0].data.size();
            }

            pDataNames->reserve(vars.size());
            pData->reserve(nLogSamples*vars.size());

            for (auto &v : vars)
            {
                pDataNames->push_back(v.name);
                //cout << v.name << " " << v.alias << " " << v.unit << " Data:");
                for (auto d : v.data)
                {
                    pData->push_back(d);
                    //cout << " " << d;
                }
                //cout << endl;
            }

            return true;
        }
        else
        {
            setLastError("Got wrong reply");
        }
    }
    return false;
}

bool RemoteHopsanClient::requestSlot(int numThreads, int &rControlPort)
{
    reqmsg_ReqServerSlots_t msg {numThreads};
    sendClientMessage<reqmsg_ReqServerSlots_t>(mpServerSocket, RequestServerSlots, msg);

    zmq::message_t response;
    if (receiveWithTimeout(*mpServerSocket, response, mShortReceiveTimeout))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyServerSlots)
        {
            replymsg_ReplyServerSlots_t msg = unpackMessage<replymsg_ReplyServerSlots_t>(response, offset, parseOK);
            if (parseOK)
            {
                rControlPort = msg.portoffset;
                assert(response.size() == offset);
                return true;
            }
            else
            {
                setLastError("Could not parse slot request reply");
            }
        }
        else if (id == NotAck)
        {
            setLastError(unpackMessage<string>(response, offset, parseOK));
        }
        else
        {
            setLastError("Got wrong reply type from server");
        }
    }
    return false;
}

bool RemoteHopsanClient::connectToWorker(int workerPort)
{
    if (workerConnected())
    {
        setLastError("You are already connected to a worker");
        return false;
    }
    else
    {
        try
        {
            mpWorkerSocket = new zmq::socket_t(*mpContext, ZMQ_REQ);
            mpWorkerSocket->setsockopt(ZMQ_LINGER, &gLinger_ms, sizeof(int));

            std::string proto, srvip, srvport;
            splitZMQAddress(mServerAddress, proto, srvip, srvport);

            if (mServerRelayIdentity.empty())
            {
                mWorkerAddress = makeZMQAddress(srvip, workerPort);
            }
            else
            {
                vector<string> fields = splitstring(mServerRelayIdentity, ".");
                string fullRelayIdentity;
                requestRelaySlot(fields.front(), workerPort, fullRelayIdentity);

                mWorkerAddress = makeZMQAddress(srvip, srvport);
                mWorkerRelayIdentity = fullRelayIdentity;
                if (!mWorkerRelayIdentity.empty())
                {
                    mpWorkerSocket->setsockopt( ZMQ_IDENTITY, (void*)mWorkerRelayIdentity.data(), mWorkerRelayIdentity.size());
                }
            }
            mpWorkerSocket->connect(mWorkerAddress.c_str());
            return true;
        }
        catch (zmq::error_t e)
        {
            setLastError( e.what() );
            deleteWorkerSocket();
        }
    }
    return false;
}


bool RemoteHopsanClient::workerConnected() const
{
    // Note we can not use socket.connected() it only checks if underlying c-socket exist
    return mpWorkerSocket && !mWorkerAddress.empty();
}

void RemoteHopsanClient::disconnectAddressServer()
{
    if (addressServerConnected())
    {
        sendShortClientMessage(mpAddressServerSocket, ClientClosing);
        receiveAckNackMessage(mpAddressServerSocket, 1000); //But we do not care about result
        try
        {
            mpAddressServerSocket->disconnect(mAddressServerAddress.c_str());
            mpAddressServerSocket->close();
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        deleteAddressServerSocket();
    }
}

void RemoteHopsanClient::disconnectWorker()
{
    if (workerConnected())
    {
        sendShortClientMessage(mpWorkerSocket, ClientClosing);
        receiveAckNackMessage(mpWorkerSocket, 1000); //But we do not care about result
        try
        {
            mpWorkerSocket->disconnect(mWorkerAddress.c_str());
            mpWorkerSocket->close();
            if (!mWorkerRelayIdentity.empty())
            {
                releaseRelaySlot(mWorkerRelayIdentity);
            }
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        mWorkerAddress.clear();
        mWorkerRelayIdentity.clear();
    }
    deleteWorkerSocket();
}

void RemoteHopsanClient::disconnectServer()
{
    if (serverConnected())
    {
        sendShortClientMessage(mpServerSocket, ClientClosing);
        receiveAckNackMessage(mpServerSocket, 1000); //But we do not care about result
        //! @todo maybe should auto disconnect when we connect to worker
        try
        {
            mpServerSocket->disconnect(mServerAddress.c_str());
            mpServerSocket->close();
            if (!mServerRelayIdentity.empty())
            {
                releaseRelaySlot(mServerRelayIdentity);
            }
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        mServerAddress.clear();
        mServerRelayIdentity.clear();
    }
    deleteServerSocket();
}

void RemoteHopsanClient::disconnect()
{
    // Disconnect from Worker
    disconnectWorker();

    // Disconnect from Server
    disconnectServer();

    // Disconnect from address server
    disconnectAddressServer();
}

bool RemoteHopsanClient::blockingSimulation(const int nLogsamples, const int logStartTime, const int simStarttime,
                                            const int simSteptime, const int simStoptime, double *pProgress)
{
    bool initOK = sendSimulateMessage(nLogsamples, logStartTime, simStarttime, simSteptime, simStoptime);
    if (initOK)
    {
        bool isAlive;
        std::thread t(&RemoteHopsanClient::requestWorkerStatusThread, this, pProgress, &isAlive);
        t.join();
	WorkerStatusT status;
	requestWorkerStatus(status);
	return (status.model_loaded && status.simualtion_success);
    }
    return initOK;
}

bool RemoteHopsanClient::requestMessages()
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendShortClientMessage(mpWorkerSocket, RequestMessages);

    zmq::message_t response;
    if (receiveWithTimeout(*mpWorkerSocket, response, mLongReceiveTimeout))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyMessages)
        {
            vector<replymsg_ReplyMessage_t> messages = unpackMessage<vector<replymsg_ReplyMessage_t>>(response, offset, parseOK);
            cout << "Received: " << messages.size() << " messages from server" << endl;
            for (size_t m=0; m<messages.size(); ++m)
            {
                cout << messages[m].type << " " << messages[m].tag << " " << messages[m].message << endl;
            }
            return true;
        }
        else
        {
            setLastError("Got wrong reply");
        }
    }
    return false;
}

bool RemoteHopsanClient::requestMessages(std::vector<char> &rTypes, std::vector<string> &rTags, std::vector<string> &rMessages)
{
    std::lock_guard<std::mutex> lock(mWorkerMutex);

    sendShortClientMessage(mpWorkerSocket, RequestMessages);

    zmq::message_t response;
    if (receiveWithTimeout(*mpWorkerSocket, response, mLongReceiveTimeout))
    {
        size_t offset=0;
        bool parseOK;
        size_t id = getMessageId(response, offset, parseOK);
        if (id == ReplyMessages)
        {
            vector<replymsg_ReplyMessage_t> messages = unpackMessage<vector<replymsg_ReplyMessage_t>>(response, offset, parseOK);
            //cout << "Received: " << messages.size() << " messages from server" << endl;
            rTypes.resize(messages.size());
            rTags.resize(messages.size());
            rMessages.resize(messages.size());
            for (size_t m=0; m<messages.size(); ++m)
            {
                rTypes[m] =  messages[m].type;
                rTags[m] = messages[m].tag;
                rMessages[m] = messages[m].message;
            }
            return true;
        }
        else
        {
            setLastError("Got wrong reply");
        }
    }
    return false;
}

bool RemoteHopsanClient::requestServerMachines(int nMachines, double maxBenchmarkTime, std::vector<ServerMachineInfoT> &rMachines)
{
    if (addressServerConnected())
    {
        reqmsg_RequestServerMachines_t req;
        req.numMachines = nMachines;
        req.maxBenchmarkTime = maxBenchmarkTime;
        req.numThreads = -1;

        sendClientMessage(mpAddressServerSocket, RequestServerMachines, req);

        zmq::message_t response;
        if (receiveWithTimeout(*mpAddressServerSocket, response, mShortReceiveTimeout))
        {
            cout << "Response size: " << response.size() << endl;
            size_t offset=0;
            bool parseOK;
            size_t id = getMessageId(response, offset, parseOK);
            if (id == ReplyServerMachines)
            {
                std::vector<replymsg_ReplyServerMachine_t> repl = unpackMessage<std::vector<replymsg_ReplyServerMachine_t>>(response,offset,parseOK);
                rMachines.reserve(repl.size());
                for (auto &rMachine : repl)
                {
                    rMachines.push_back(rMachine);
                }
                return true;
            }
            else
            {
                setLastError("Got wrong reply");
            }
        }
    }
    return false;
}

bool RemoteHopsanClient::requestRelaySlot(const std::string &rBaseRelayIdentity, const int port, std::string &rRelayIdentityFull)
{
    if (addressServerConnected())
    {
        std::vector<std::string> fields = splitstring(rBaseRelayIdentity, "."); // Split in case socket id included (we dont want that)
        reqmsg_RelaySlot_t msg {fields.front(), port};
        sendClientMessage(mpAddressServerSocket, RequestRelaySlot, msg);

        zmq::message_t response;
        if (receiveWithTimeout(*mpAddressServerSocket, response, mShortReceiveTimeout))
        {
            size_t offset=0;
            bool parseOK;
            size_t id = getMessageId(response, offset, parseOK);
            if (id == ReplyRelaySlot)
            {
                rRelayIdentityFull = unpackMessage<std::string>(response,offset,parseOK);
                return true;
            }
            else if (id == NotAck)
            {
                setLastError(unpackMessage<std::string>(response, offset, parseOK));
            }
            else
            {
                setLastError("Received wrong reply");
            }
        }
    }
    rRelayIdentityFull.clear();
    return false;
}

bool RemoteHopsanClient::releaseRelaySlot(const string &rRelayIdentityFull)
{
    if (addressServerConnected())
    {
        sendClientMessage(mpAddressServerSocket, ReleaseRelaySlot, rRelayIdentityFull);
        string err;
        bool rc = receiveAckNackMessage(mpAddressServerSocket, mShortReceiveTimeout, err);
        if (!rc)
        {
            setLastError(err);
        }
        return rc;
    }
    return false;
}

string RemoteHopsanClient::getLastErrorMessage() const
{
    return mLastErrorMessage;
}

bool RemoteHopsanClient::receiveWithTimeout(zmq::socket_t &rSocket, zmq::message_t &rMessage, long timeout)
{
    // Create a poll item
    std::vector<zmq::pollitem_t> pollitems {{ (void*)rSocket, 0, ZMQ_POLLIN, 0 }};
    try
    {
        // Poll socket for a reply, with timeout
        zmq::poll(pollitems,  timeout);
        // If we have received a message then read message and return true
        if (pollitems[0].revents & ZMQ_POLLIN)
        {
            rSocket.recv(&rMessage);
            return true;
        }
        // Else we reached timeout, return false
        else
        {
            setLastError("Timeout in receive");
        }
    }
    catch(zmq::error_t e)
    {
        setLastError(e.what());
    }
    return false;
}

void RemoteHopsanClient::deleteSockets()
{
    deleteAddressServerSocket();
    deleteServerSocket();
    deleteWorkerSocket();
}

void RemoteHopsanClient::deleteAddressServerSocket()
{
    if (mpAddressServerSocket)
    {
        try
        {
            delete mpAddressServerSocket;
            cout << "deleted address server socket" << endl;
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        mpAddressServerSocket = nullptr;
    }
    mAddressServerAddress.clear();
}

void RemoteHopsanClient::deleteServerSocket()
{
    if (mpServerSocket)
    {
        try
        {
            delete mpServerSocket;
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        mpServerSocket = nullptr;
    }
    mServerRelayIdentity.clear();
    mServerAddress.clear();
}

void RemoteHopsanClient::deleteWorkerSocket()
{
    if (mpWorkerSocket)
    {
        try
        {
            delete mpWorkerSocket;
        }
        catch(zmq::error_t e)
        {
            setLastError(e.what());
        }
        mpWorkerSocket = nullptr;
    }
    mWorkerRelayIdentity.clear();
    mWorkerAddress.clear();
}

void RemoteHopsanClient::requestWorkerStatusThread(double *pProgress, bool *pAlive)
{
    const int defaultSleepMs = 200;
    double progressedTime = 0;
    double lastProgress=0;
    bool firstNoProgress = true;

    // Assume server alive
    // Note! This one should be set true from the outside since it may take some time to launch this thread
    *pAlive = true;

    WorkerStatusT status;
    chrono::milliseconds msd{defaultSleepMs};
    chrono::milliseconds ms=msd;
    chrono::steady_clock::time_point startT = chrono::steady_clock::now();
    chrono::steady_clock::time_point lastNoProgressTime = chrono::steady_clock::now();

    bool rc = requestWorkerStatus(status);
    *pProgress = status.simulation_progress;
    *pAlive = rc;
    while (!status.simulation_finished && rc)
    {
        std::this_thread::sleep_for(ms);
        progressedTime = elapsedSecondsSince(startT);
        rc = requestWorkerStatus(status);
        *pProgress = status.simulation_progress;
        // Ok make an estimate of remaning time based on progressed time
        if (status.simulation_progress > 1e-6)
        {
            // Calulate an estimate of the remaining time
            double  remaining_time = progressedTime / status.simulation_progress - progressedTime;

            // Make sure that we request status within at most mMaxWorkerStatusRequestWaitTime seconds
            // If remaning time lower, then wait estimated remaning time + 100 ms
            double sleep_time = min( remaining_time+0.1, mMaxWorkerStatusRequestWaitTime );
            if (sleep_time > 0 )
            {
                std::chrono::milliseconds mss{int(sleep_time*1000.0)};
                ms = mss;
            }
            else
            {
                ms = msd;
            }

            if (status.simulation_progress == lastProgress)
            {
                if (firstNoProgress)
                {
                    lastNoProgressTime = chrono::steady_clock::now();
                    firstNoProgress = false;
                }
                else if (elapsedSecondsSince(lastNoProgressTime) > mMaxNoProgressTime)
                {
                    *pAlive = false;
                    break;
                }
            }
            else
            {
                firstNoProgress = true;
            }

            lastProgress = status.simulation_progress;
        }
    }
}

void RemoteHopsanClient::setLastError(const string &rError)
{
    mLastErrorMessage = rError;
    cout << "RemoteHopsanClient Error: " << rError << endl;
}


RemoteHopsanClient::RemoteHopsanClient(zmq::context_t &rContext)
{
//    int linger_ms = 1000;
//    try
//    {
        mpContext = &rContext;
//        mpServerSocket = new zmq::socket_t(rContext, ZMQ_REQ);
//        mpWorkerSocket = new zmq::socket_t(rContext, ZMQ_REQ);
//        mpServerSocket->setsockopt(ZMQ_LINGER, &linger_ms, sizeof(int));
//        mpWorkerSocket->setsockopt(ZMQ_LINGER, &linger_ms, sizeof(int));
//    }
//    catch(zmq::error_t e)
//    {
//        deleteSockets();
//        setLastError(e.what());
//    }
}

RemoteHopsanClient::~RemoteHopsanClient()
{
    disconnect();
    deleteSockets();
}

bool RemoteHopsanClient::areSocketsValid() const
{
    return mpServerSocket && mpWorkerSocket;
}

void RemoteHopsanClient::setShortReceiveTimeout(long ms)
{
    mShortReceiveTimeout = ms;
}

void RemoteHopsanClient::setLongReceiveTimeout(long ms)
{
    mLongReceiveTimeout = ms;
}

long RemoteHopsanClient::getShortReceiveTimeout() const
{
    return mShortReceiveTimeout;
}

long RemoteHopsanClient::getLongReceiveTimeout() const
{
    return mLongReceiveTimeout;
}

void RemoteHopsanClient::setMaxWorkerStatusRequestWaitTime(double seconds)
{
    mMaxWorkerStatusRequestWaitTime = seconds;
}

bool RemoteHopsanClient::connectToAddressServer(string address)
{
    if (addressServerConnected())
    {
        setLastError("You are already connected to an address server");
        return false;
    }
    else
    {
        if (!mpAddressServerSocket)
        {
            int linger_ms = 1000;
            try
            {
                mpAddressServerSocket = new zmq::socket_t(*mpContext, ZMQ_REQ);
                mpAddressServerSocket->setsockopt(ZMQ_LINGER, &linger_ms, sizeof(int));
            }
            catch(zmq::error_t e)
            {
                setLastError(e.what());
                deleteAddressServerSocket();
            }
        }

        if (mpAddressServerSocket)
        {
            string ip,port,relayid;
            splitaddress(address, ip,port,relayid);
            try
            {
                mAddressServerAddress = makeZMQAddress(ip,port);
                mpAddressServerSocket->connect(mAddressServerAddress.c_str());
                return true;
            }
            catch (zmq::error_t e)
            {
                setLastError(e.what());
                disconnectAddressServer();
            }
        }
        return false;
    }
}

bool RemoteHopsanClient::addressServerConnected() const
{
    // Note we can not use socket.connected() it only checks if underlying c-socket exist
    return mpAddressServerSocket && !mAddressServerAddress.empty();
}
