#include "NumHopScriptDialog.h"

#include <QVBoxLayout>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>

#include "GUIObjects/GUIContainerObject.h"
#include "MessageHandler.h"
#include "global.h"

NumHopScriptDialog::NumHopScriptDialog(ContainerObject *pSystem, QWidget *pParent) :
    QDialog(pParent)
{
    mpSystem = pSystem;

    setObjectName("NumHop Script Dialog");
    resize(640,480);
    setWindowTitle(tr("NumHop Script Dialog"));
    setAttribute(Qt::WA_DeleteOnClose, true);
    //setPalette(QPalette(QColor("gray"), QColor("whitesmoke")));

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    mpTextEdit = new QTextEdit(this);

    QDialogButtonBox *pButtonBox =    new QDialogButtonBox(Qt::Horizontal, this);
    QPushButton *pCancelButton = new QPushButton(tr("&Cancel"), this);
    QPushButton *pRevertButton = new QPushButton(tr("&Revert"), this);
    QPushButton *pOkButton = new QPushButton(tr("&Ok"), this);
    QPushButton *pApplyButton = new QPushButton(tr("&Apply"), this);
    QPushButton *pRunButton = new QPushButton(tr("&Run"), this);
    pOkButton->setDefault(true);
    pButtonBox->addButton(pRunButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(pApplyButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(pOkButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(pRevertButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(pCancelButton, QDialogButtonBox::ActionRole);

    connect(pApplyButton, SIGNAL(clicked()), this, SLOT(applyPressed()));
    connect(pOkButton, SIGNAL(clicked()), this, SLOT(okPressed()));
    connect(pCancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(pRevertButton, SIGNAL(clicked()), this, SLOT(revert()));
    connect(pRunButton, SIGNAL(clicked()), this, SLOT(run()));
    connect(pSystem, SIGNAL(objectDeleted()), this, SLOT(close()));

    pLayout->addWidget(mpTextEdit);
    pLayout->addWidget(pButtonBox);

    // Set text contents
    revert();

}

void NumHopScriptDialog::applyPressed()
{
    if (mpSystem)
    {
        mpSystem->setNumHopScript(mpTextEdit->toPlainText());
    }
}

void NumHopScriptDialog::okPressed()
{
    if (mpSystem)
    {
        mpSystem->setNumHopScript(mpTextEdit->toPlainText());
    }
    close();
}

void NumHopScriptDialog::revert()
{
    if (mpSystem)
    {
        mpTextEdit->setText(mpSystem->getNumHopScript());
    }
    else
    {
        mpTextEdit->setText("Error: System is no longer present!");
    }
}

void NumHopScriptDialog::run()
{
    QString output;
    if (mpSystem)
    {
        mpSystem->runNumHopScript(mpTextEdit->toPlainText(), true, output);
        output.prepend("Running NumHop\n");
        gpMessageHandler->addInfoMessage(output);
    }
    else
    {
        gpMessageHandler->addErrorMessage("NumHopScriptDialog::run() System is no longer present!");
    }
}

