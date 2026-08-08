#include "gui/RulesDialog.hpp"

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>

RulesDialog::RulesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Правила игры"));
    resize(620, 560);

    auto *layout = new QVBoxLayout(this);

    auto *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(false);

    QFile file(QString(ASSETS_DIR) + QStringLiteral("/rules.md"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString markdown = in.readAll();
        browser->setMarkdown(markdown);
    }
    else
    {
        browser->setPlainText(
            QStringLiteral("Не удалось загрузить assets/rules.md."));
    }

    layout->addWidget(browser);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
