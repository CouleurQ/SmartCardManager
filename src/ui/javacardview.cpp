#include "javacardview.h"

#include "card/gpcoding.h"
#include "card/gpdemodata.h"
#include "gpauthdialog.h"
#include "gpsendpanel.h"
#include "utility.h"

#include <algorithm>
#include <functional>

#include <QCryptographicHash>
#include <QFontDatabase>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

enum ItemKind { Placeholder = 0, Group, Entry, Module };

const int KindRole = Qt::UserRole;
const int EntryRole = Qt::UserRole + 1;     // index into the registry or the data objects
const int AidRole = Qt::UserRole + 2;       // AID of a module node

QString aidText(const QByteArray &aid)
{
    return Utility::byteArrayToHexString(aid).remove(' ');
}


QString hexValue(quint8 value)
{
    return QString("%1").arg(value, 2, 16, QChar('0')).toUpper();
}


QString kindText(GpRegistryEntry::Kind kind)
{
    switch (kind) {
    case GpRegistryEntry::Kind::IssuerSecurityDomain: return "Issuer Security Domain";
    case GpRegistryEntry::Kind::SecurityDomain: return "Supplementary Security Domain";
    case GpRegistryEntry::Kind::ExecutableLoadFile: return "Executable Load File";
    case GpRegistryEntry::Kind::Application: break;
    }
    return "Application";
}


QTreeWidget *createTree(const QString &objectName, const QStringList &headers)
{
    auto *tree = new QTreeWidget;
    tree->setObjectName(objectName);
    tree->setColumnCount(headers.size());
    tree->setHeaderLabels(headers);
    tree->setAlternatingRowColors(true);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < headers.size(); ++i)
        tree->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    return tree;
}


QString channelText(int channel)
{
    return channel == 0 ? QString("0 - Basic channel") : QString("%1 - Logical channel").arg(channel);
}


QWidget *titled(const QString &title, QWidget *content, QLayout *footer = nullptr)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(QString("<b>%1</b>").arg(title)));
    layout->addWidget(content, 1);
    if (footer)
        layout->addLayout(footer);
    return widget;
}


void setPlaceholder(QTreeWidget *tree, const QString &text)
{
    tree->clear();
    auto *item = new QTreeWidgetItem(tree, {text});
    item->setData(0, KindRole, Placeholder);
    item->setFlags(Qt::NoItemFlags);
    QFont font = item->font(0);
    font.setItalic(true);
    item->setFont(0, font);
}


void makeBold(QTreeWidgetItem *item)
{
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);
}

} // namespace


JavaCardView::JavaCardView(CardSession *cardSession, QWidget *parent)
    : QWidget(parent)
    , session(cardSession)
{
    setObjectName("javaCardView");

    auto link = [](const QString &name, const QString &text, const QString &tip) {
        auto *b = new QPushButton(text);
        b->setObjectName(name);
        b->setToolTip(tip);
        b->setFlat(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::TabFocus);
        b->setStyleSheet("QPushButton { border: none; padding: 3px 0; text-align: left; text-decoration: underline;"
                         " color: palette(link); } QPushButton:disabled { color: palette(mid); }");
        return b;
    };
    buttonOpenChannel = link("buttonOpenChannel", "Open Logical Channel", "MANAGE CHANNEL [open]");
    buttonCloseChannel = link("buttonCloseChannel", "Close Logical Channel", "MANAGE CHANNEL [close] the working channel");
    channelCombo = new QComboBox;
    channelCombo->setObjectName("channelCombo");
    channelCombo->setToolTip("Channel all actions work on");
    channelCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    channelCombo->setStyleSheet("QComboBox { border: none; background: transparent; padding: 0 4px; }"
                                "QComboBox:disabled { color: palette(mid); }");
    buttonAuthenticate = link("buttonAuthenticate", "Open Secure Channel...", "INITIALIZE UPDATE, EXTERNAL AUTHENTICATE");
    buttonSendApdu = link("buttonSendApdu", "Send APDU...", "Send a command built from CLA INS P1 P2 Lc data Le");
    buttonSendBytes = link("buttonSendBytes", "Send Bytes...", "Send raw bytes");
    buttonRefresh = link("buttonRefresh", "Refresh Card Content", "GET STATUS, GET DATA");
    buttonLoad = link("buttonLoad", "Load...", "INSTALL [for load], LOAD");
    buttonInstall = link("buttonInstall", "Install...", "INSTALL [for install and make selectable]");
    buttonDelete = link("buttonDelete", "Delete...", "DELETE the selected Application, Security Domain or Executable Load File");
    buttonSetStatus = link("buttonSetStatus", "Set Status...", "SET STATUS of the selected object");
    buttonPutKey = link("buttonPutKey", "Put Key...", "PUT KEY");
    buttonDeleteKey = link("buttonDeleteKey", "Delete Key...", "DELETE the selected key (set)");

    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.85);

    detailsToggle = new QToolButton;
    detailsToggle->setObjectName("detailsToggle");
    detailsToggle->setFont(small);
    detailsToggle->setCheckable(true);
    detailsToggle->setAutoRaise(true);
    detailsToggle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    detailsToggle->setStyleSheet("QToolButton { border: none; background: transparent; padding: 3px 0; text-align: left; }"
                                 "QToolButton:disabled { color: palette(text); }");
    detailsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    detailsText = new QPlainTextEdit;
    detailsText->setObjectName("secureChannelDetails");
    detailsText->setReadOnly(true);
    detailsText->setStyleSheet("QPlainTextEdit { background: palette(window); }");
    detailsText->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSizeF(small.pointSizeF() * 0.95);
    detailsText->setFont(mono);
    detailsText->setMinimumHeight(140);

    auto heading = [](const QString &text) -> QLabel * {
        auto *l = new QLabel(QString("<b>%1</b>").arg(text));
        l->setContentsMargins(0, 10, 0, 2);
        return l;
    };

    actionsPage = new QWidget;
    actionsPage->setObjectName("actionsPanel");
    auto *actionsLayout = new QVBoxLayout(actionsPage);
    actionsLayout->setContentsMargins(16, 0, 16, 0);
    actionsLayout->setSpacing(0);
    // the first heading sits where the title of a sub page sits (below the Back row)
    actionsLayout->addSpacing(QLabel("<b>Registry</b>").sizeHint().height() + 12);

    auto *actionsBody = new QVBoxLayout;
    actionsBody->setContentsMargins(0, 0, 0, 0);
    auto section = [&](const QString &title, bool first = false) {
        QLabel *h = heading(title);
        if (first)
            h->setContentsMargins(0, 0, 0, 2);
        actionsBody->addWidget(h);
        auto *items = new QVBoxLayout;
        items->setContentsMargins(16, 0, 0, 0);
        actionsBody->addLayout(items);
        return items;
    };

    QVBoxLayout *items = section("Logical channels", true);
    items->addWidget(buttonOpenChannel);
    items->addWidget(buttonCloseChannel);
    auto *channelRow = new QHBoxLayout;
    auto *channelCaption = new QLabel("Working channel");
    channelCaption->setFont(small);
    channelCombo->setFont(small);
    channelRow->addWidget(channelCaption);
    channelRow->addWidget(channelCombo);
    channelRow->addStretch();
    items->addLayout(channelRow);
    items = section("Secure channel");
    items->addWidget(buttonAuthenticate);
    items->addWidget(detailsToggle, 0, Qt::AlignLeft);
    items->addWidget(detailsText);
    items = section("Commands");
    items->addWidget(buttonSendApdu);
    items->addWidget(buttonSendBytes);
    items = section("Card content");
    for (QPushButton *b : {buttonRefresh, buttonLoad, buttonInstall, buttonDelete, buttonSetStatus})
        items->addWidget(b);
    items = section("Keys");
    items->addWidget(buttonPutKey);
    items->addWidget(buttonDeleteKey);
    actionsLayout->addLayout(actionsBody);
    actionsLayout->addStretch();

    registryTree = createTree("registryTree", {"AID", "Name"});
    registryTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    registryTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    detailsTree = createTree("detailsTree", {"Property", "Value"});
    detailsTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    detailsTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    buttonPropSetStatus = link("buttonPropSetStatus", "Set Status...", "SET STATUS of this object");
    buttonPropDelete = link("buttonPropDelete", "Delete...", "DELETE this object");
    buttonPropInstall = link("buttonPropInstall", "Install from Package...", "INSTALL [for install and make selectable] from this Executable Load File");
    buttonPropRefresh = link("buttonPropRefresh", "Refresh", "Read the registry again");

    auto *props = new QWidget;
    auto *propsLayout = new QVBoxLayout(props);
    propsLayout->setContentsMargins(0, 0, 0, 0);
    propsLayout->addWidget(detailsTree, 1);
    propsLayout->addWidget(heading("Actions"));
    for (QPushButton *b : {buttonPropInstall, buttonPropSetStatus, buttonPropDelete, buttonPropRefresh})
        propsLayout->addWidget(b);

    keyTree = createTree("keyTree", {"Key", "Type", "Length", "Usage", "Access"});

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName("javaCardSplitter");
    splitter->addWidget(titled("Registry", registryTree));
    authPanel = new GpAuthDialog;
    authPanel->setWindowFlags(Qt::Widget);
    apduPanel = new GpSendPanel(GpSendPanel::Mode::Apdu);
    apduPanel->setObjectName("apduPanel");
    bytesPanel = new GpSendPanel(GpSendPanel::Mode::Bytes);
    bytesPanel->setObjectName("bytesPanel");

    stack = new QStackedWidget;
    stack->setObjectName("middleStack");
    stack->addWidget(actionsPage);
    stack->addWidget(makePage("Open Secure Channel", authPanel));
    stack->addWidget(makePage("Send APDU", apduPanel));
    stack->addWidget(makePage("Send Bytes", bytesPanel));
    propertiesPage = makePage("Properties", props);
    stack->addWidget(propertiesPage);
    splitter->addWidget(stack);
    splitter->addWidget(titled("Keys", keyTree));
    splitter->setSizes({330, 500, 170});
    splitter->setStretchFactor(0, 33);
    splitter->setStretchFactor(1, 50);
    splitter->setStretchFactor(2, 17);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(16);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(splitter, 1);

    connect(authPanel, &QDialog::accepted, this, &JavaCardView::applyAuthentication);
    connect(authPanel, &QDialog::rejected, this, &JavaCardView::showActions);
    connect(buttonSendApdu, &QPushButton::clicked, this, [this] { stack->setCurrentWidget(apduPanel->parentWidget()); });
    connect(buttonSendBytes, &QPushButton::clicked, this, [this] { stack->setCurrentWidget(bytesPanel->parentWidget()); });
    connect(apduPanel, &GpSendPanel::sendRequested, this, [this](const QByteArray &c) { sendCommand(c, apduPanel); });
    connect(bytesPanel, &GpSendPanel::sendRequested, this, [this](const QByteArray &c) { sendCommand(c, bytesPanel); });
    connect(session, &CardSession::responseReceived, this, [this](const QByteArray &, const QByteArray &r) { onResponseReceived(r); });
    connect(detailsToggle, &QToolButton::toggled, this, &JavaCardView::setDetailsVisible);
    connect(buttonAuthenticate, &QPushButton::clicked, this, &JavaCardView::authenticate);
    connect(buttonOpenChannel, &QPushButton::clicked, this, &JavaCardView::openLogicalChannel);
    connect(buttonCloseChannel, &QPushButton::clicked, this, &JavaCardView::closeLogicalChannel);
    connect(channelCombo, &QComboBox::activated, this, [this](int index) { selectChannel(channelCombo->itemData(index).toInt()); });
    connect(buttonRefresh, &QPushButton::clicked, this, &JavaCardView::refresh);
    connect(buttonLoad, &QPushButton::clicked, this, [this] { notImplemented("Load"); });
    connect(buttonInstall, &QPushButton::clicked, this, [this] { notImplemented("Install"); });
    connect(buttonDelete, &QPushButton::clicked, this, [this] { notImplemented("Delete"); });
    connect(buttonSetStatus, &QPushButton::clicked, this, [this] { notImplemented("Set Status"); });
    connect(buttonPutKey, &QPushButton::clicked, this, [this] { notImplemented("Put Key"); });
    connect(buttonDeleteKey, &QPushButton::clicked, this, [this] { notImplemented("Delete Key"); });

    connect(registryTree, &QTreeWidget::currentItemChanged, this, [this] {
        if (stack->currentWidget() == propertiesPage)
            showEntryDetails();
        updateActions();
    });
    connect(registryTree, &QTreeWidget::itemDoubleClicked, this, &JavaCardView::openProperties);
    connect(buttonPropSetStatus, &QPushButton::clicked, this, [this] { notImplemented("Set Status"); });
    connect(buttonPropDelete, &QPushButton::clicked, this, [this] { notImplemented("Delete"); });
    connect(buttonPropInstall, &QPushButton::clicked, this, [this] { notImplemented("Install"); });
    connect(buttonPropRefresh, &QPushButton::clicked, this, [this] { refresh(); showActions(); });
    connect(keyTree, &QTreeWidget::currentItemChanged, this, &JavaCardView::updateActions);

    reset();
}


void JavaCardView::setConnected(bool isConnected)
{
    if (connected == isConnected)
        return;
    connected = isConnected;
    if (!connected)
        reset();
    updateActions();
}


void JavaCardView::reset()
{
    for (ChannelState &state : channels)
        state = ChannelState();
    channels[0].open = true;
    channel = 0;
    pendingOp = PendingChannelOp::None;

    authenticated = false;
    content = GpCardContent();
    detailsToggle->setText("Not authenticated");
    detailsToggle->setChecked(false);
    setDetailsVisible(false);
    showPlaceholders("Open the secure channel to read the card content");
    sendingPanel = nullptr;
    apduPanel->clearResponse();
    bytesPanel->clearResponse();
    showActions();
    updateActions();
}


void JavaCardView::showPlaceholders(const QString &text)
{
    setPlaceholder(registryTree, text);
    setPlaceholder(keyTree, text);
    detailsTree->clear();
    detailsText->clear();
}


void JavaCardView::showCard(const GpAuthParams &params, const GpCardContent &card)
{
    auth = params;
    content = card;
    authenticated = true;

    const QString domain = auth.selectFirst ? Utility::byteArrayToHexString(auth.targetAid) : QString("selected application");
    detailsToggle->setText(QString("Open (simulated): %1, %2, %3")
                           .arg(domain, GpAuthParams::protocolText(auth.protocol),
                                GpCoding::securityLevelText(auth.securityLevel)));

    showSecureChannelDetails();
    if (content.registry.isEmpty() && content.keys.isEmpty()) {
        showPlaceholders("Use Refresh to read the card content");
    } else {
        populateRegistry();
        populateKeys();
    }
    updateActions();
}


void JavaCardView::authenticate()
{
    authPanel->setParams(auth);
    stack->setCurrentWidget(authPanel->parentWidget());
    authPanel->show();      // accept() and reject() hide the embedded dialog
}


void JavaCardView::applyAuthentication()
{
    const GpAuthParams params = authPanel->params();
    showCard(params, params.readRegistry ? GpDemoData::content(params) : GpCardContent());
    status(QString("Open Secure Channel on channel %1: simulated, no command was sent to the card").arg(channel));
    showActions();
}


void JavaCardView::showActions()
{
    stack->setCurrentWidget(actionsPage);
}


QWidget *JavaCardView::makePage(const QString &title, QWidget *content)
{
    auto *name = new QLabel(QString("<b>%1</b>").arg(title));
    name->setContentsMargins(0, 12, 0, 12);

    auto *back = new QPushButton("< Back");
    back->setObjectName("buttonBack");
    back->setFlat(true);
    back->setCursor(Qt::PointingHandCursor);
    back->setStyleSheet("QPushButton { border: none; font-weight: bold; color: black; padding: 0; text-align: left; }");
    back->setFixedHeight(QLabel("<b>Registry</b>").sizeHint().height());
    back->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    connect(back, &QPushButton::clicked, this, &JavaCardView::showActions);

    auto *header = new QVBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(0);
    header->addWidget(back, 0, Qt::AlignLeft);
    header->addWidget(name);

    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->addLayout(header);
    layout->addWidget(content, 1);
    return page;
}


void JavaCardView::sendCommand(const QByteArray &command, GpSendPanel *panel)
{
    if (!connected)
        return;
    if (pendingOp != PendingChannelOp::None)
        return;
    sendingPanel = panel;
    panel->clearResponse();
    const QByteArray cmd = mapCommand(command);
    emit logBytes(LogType::CardCmd, cmd);
    session->sendAsync(cmd);
}


void JavaCardView::onResponseReceived(const QByteArray &response)
{
    if (pendingOp != PendingChannelOp::None) {
        handleManageChannel(response);
        return;
    }
    if (!sendingPanel)
        return;
    sendingPanel->showResponse(response);
    sendingPanel = nullptr;
    if (response.size() >= 2)
        emit logBytes(LogType::CardRsp, response);
    else
        emit logString(LogType::Error, "No response from card");
}


void JavaCardView::refresh()
{
    showCard(auth, GpDemoData::content(auth));
    status("Refresh: simulated, no command was sent to the card");
}


// Every command sent from this tab goes through here: logical channel now, the secure channel envelope
// once the secure channel is real (Send Command and Run Script never map)
QByteArray JavaCardView::mapCommand(QByteArray command) const
{
    // ISO 7816-4: bits b2b1 of the first interindustry/proprietary CLA select channels 0-3
    if (channel > 0 && !command.isEmpty() && !(command[0] & 0x40))
        command[0] = char((command[0] & ~0x03) | channel);
    return command;
}


void JavaCardView::openLogicalChannel()
{
    if (!connected || pendingOp != PendingChannelOp::None)
        return;
    pendingOp = PendingChannelOp::Open;
    const QByteArray cmd = QByteArray::fromHex("0070000001");
    emit logBytes(LogType::CardCmd, cmd);
    session->sendAsync(cmd);
}


void JavaCardView::closeLogicalChannel()
{
    if (!connected || channel == 0 || pendingOp != PendingChannelOp::None)
        return;
    pendingOp = PendingChannelOp::Close;
    pendingChannel = channel;
    QByteArray cmd = QByteArray::fromHex("00708000");
    cmd[3] = char(channel);
    emit logBytes(LogType::CardCmd, cmd);
    session->sendAsync(cmd);
}


void JavaCardView::handleManageChannel(const QByteArray &response)
{
    const PendingChannelOp op = pendingOp;
    pendingOp = PendingChannelOp::None;

    if (response.size() < 2) {
        emit logString(LogType::Error, "No response from card");
        status("MANAGE CHANNEL: no response from the card");
        return;
    }
    emit logBytes(LogType::CardRsp, response);

    const bool ok = response.right(2) == QByteArray::fromHex("9000");
    if (!ok) {
        status(QString("MANAGE CHANNEL failed: SW %1").arg(Utility::byteArrayToHexString(response.right(2))));
        return;
    }

    if (op == PendingChannelOp::Open) {
        const int opened = response.size() > 2 ? quint8(response[0]) : 0;
        if (opened < 1 || opened > 3 || channels[opened].open) {
            status("MANAGE CHANNEL: the card returned an unusable channel number");
            return;
        }
        channels[opened].open = true;
        selectChannel(opened);
        status(QString("Logical channel %1 opened").arg(opened));
    } else {
        const bool wasWorking = pendingChannel == channel;
        channels[pendingChannel] = ChannelState();
        if (wasWorking) {
            channel = 0;
            auth = channels[0].auth;
            content = channels[0].content;
            authenticated = channels[0].authenticated;
            showChannel();
        } else {
            updateActions();
        }
        status(QString("Logical channel %1 closed").arg(pendingChannel));
    }
}


void JavaCardView::selectChannel(int newChannel)
{
    if (newChannel == channel || newChannel < 0 || newChannel > 3 || !channels[newChannel].open)
        return;
    channels[channel].auth = auth;
    channels[channel].content = content;
    channels[channel].authenticated = authenticated;

    channel = newChannel;
    auth = channels[channel].auth;
    content = channels[channel].content;
    authenticated = channels[channel].authenticated;
    showChannel();
}


void JavaCardView::showChannel()
{
    sendingPanel = nullptr;
    apduPanel->clearResponse();
    bytesPanel->clearResponse();
    if (authenticated) {
        showCard(auth, content);
    } else {
        detailsToggle->setText("Not authenticated");
        detailsToggle->setChecked(false);
        showPlaceholders("Open the secure channel to read the card content");
        setDetailsVisible(false);
        updateActions();
    }
}


void JavaCardView::updateChannelUi()
{
    channelCombo->blockSignals(true);
    channelCombo->clear();
    int openCount = 0;
    for (int i = 0; i < 4; ++i) {
        if (!channels[i].open)
            continue;
        ++openCount;
        channelCombo->addItem(channelText(i), i);
        if (i == channel)
            channelCombo->setCurrentIndex(channelCombo->count() - 1);
    }
    channelCombo->blockSignals(false);
    int textWidth = 0;
    for (int i = 0; i < channelCombo->count(); ++i)
        textWidth = std::max(textWidth, channelCombo->fontMetrics().horizontalAdvance(channelCombo->itemText(i)));
    channelCombo->setFixedWidth(textWidth + 48);     // text, padding and the drop-down button

    channelCombo->setEnabled(connected && openCount > 1);
    buttonOpenChannel->setEnabled(connected && openCount < 4);
    buttonCloseChannel->setEnabled(connected && channel != 0);
}


void JavaCardView::status(const QString &text)
{
    emit logString(LogType::Info, text);
}


QString JavaCardView::secureChannelText() const
{
    return authenticated ? GpAuthParams::protocolText(auth.protocol) : QString("off");
}


void JavaCardView::notImplemented(const QString &action)
{
    status(QString("%1: not implemented yet").arg(action));
}


QTreeWidgetItem *JavaCardView::addEntry(QTreeWidgetItem *parent, int index)
{
    const GpRegistryEntry &e = content.registry.at(index);
    const QString aid = aidText(e.aid);

    auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(registryTree);
    item->setText(0, aid);
    item->setText(1, e.label);
    item->setData(0, KindRole, Entry);
    item->setData(0, EntryRole, index);
    item->setToolTip(0, aid);

    for (const QByteArray &module : e.moduleAids) {
        auto *child = new QTreeWidgetItem(item);
        child->setText(0, aidText(module));
        child->setData(0, KindRole, Module);
        child->setData(0, AidRole, module);
        child->setData(0, EntryRole, index);
    }
    return item;
}


void JavaCardView::populateRegistry()
{
    registryTree->clear();
    const QList<GpRegistryEntry> &entries = content.registry;
    QSet<int> placed;

    // Security Domains contain the applications and Security Domains associated with them
    std::function<void(QTreeWidgetItem *, int)> addDomainContent = [&](QTreeWidgetItem *item, int index) {
        for (int j = 0; j < entries.size(); ++j) {
            const GpRegistryEntry &child = entries.at(j);
            if (j == index || placed.contains(j) || child.kind == GpRegistryEntry::Kind::ExecutableLoadFile
                || child.kind == GpRegistryEntry::Kind::IssuerSecurityDomain || child.domainAid != entries.at(index).aid)
                continue;
            placed.insert(j);
            QTreeWidgetItem *childItem = addEntry(item, j);
            if (child.kind == GpRegistryEntry::Kind::SecurityDomain)
                addDomainContent(childItem, j);
        }
        item->setExpanded(true);
    };

    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).kind != GpRegistryEntry::Kind::IssuerSecurityDomain)
            continue;
        placed.insert(i);
        addDomainContent(addEntry(nullptr, i), i);
    }
    for (int i = 0; i < entries.size(); ++i) {     // entries whose domain is not on the card
        const GpRegistryEntry &e = entries.at(i);
        if (placed.contains(i) || e.kind == GpRegistryEntry::Kind::ExecutableLoadFile)
            continue;
        placed.insert(i);
        QTreeWidgetItem *item = addEntry(nullptr, i);
        if (e.kind == GpRegistryEntry::Kind::SecurityDomain)
            addDomainContent(item, i);
    }

    int loadFiles = 0;
    for (const GpRegistryEntry &e : entries)
        loadFiles += e.kind == GpRegistryEntry::Kind::ExecutableLoadFile;
    auto *group = new QTreeWidgetItem(registryTree);
    group->setText(0, QString("Executable Load Files (%1)").arg(loadFiles));
    group->setData(0, KindRole, Group);
    makeBold(group);
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).kind == GpRegistryEntry::Kind::ExecutableLoadFile)
            addEntry(group, i);
    }
    group->setExpanded(true);

    registryTree->setCurrentItem(registryTree->topLevelItem(0));
}


void JavaCardView::populateKeys()
{
    keyTree->clear();
    QList<GpKeyInfo> keys = content.keys;
    std::stable_sort(keys.begin(), keys.end(), [](const GpKeyInfo &a, const GpKeyInfo &b) {
        return a.version != b.version ? a.version < b.version : a.id < b.id;
    });

    QMap<quint8, QTreeWidgetItem *> sets;
    for (const GpKeyInfo &k : keys) {
        QTreeWidgetItem *set = sets.value(k.version);
        if (!set) {
            set = new QTreeWidgetItem(keyTree);
            set->setData(0, KindRole, Group);
            makeBold(set);
            sets.insert(k.version, set);
        }

        QStringList types, lengths;
        for (const GpKeyInfo::Component &c : k.components) {
            types << GpCoding::keyTypeText(c.type);
            lengths << QString("%1 bit").arg(c.length * 8);
        }
        auto *item = new QTreeWidgetItem(set);
        item->setText(0, QString("KID %1").arg(hexValue(k.id)));
        item->setText(1, types.join(", "));
        item->setText(2, lengths.join(", "));
        item->setText(3, k.hasUsage ? GpCoding::keyUsageText(k.usage) : QString("-"));
        item->setText(4, k.hasAccess ? GpCoding::keyAccessText(k.access) : QString("-"));
        item->setData(0, KindRole, Entry);
    }

    for (auto it = sets.cbegin(); it != sets.cend(); ++it) {
        it.value()->setText(0, QString("Key version %1 (%2 keys)").arg(hexValue(it.key())).arg(it.value()->childCount()));
        it.value()->setExpanded(true);
    }
}


const GpRegistryEntry *JavaCardView::currentEntry() const
{
    const QTreeWidgetItem *item = registryTree->currentItem();
    if (!item)
        return nullptr;
    const int kind = item->data(0, KindRole).toInt();
    if (kind != Entry && kind != Module)
        return nullptr;
    const int index = item->data(0, EntryRole).toInt();
    return index >= 0 && index < content.registry.size() ? &content.registry.at(index) : nullptr;
}


void JavaCardView::openProperties(QTreeWidgetItem *item)
{
    const int kind = item->data(0, KindRole).toInt();
    if (kind != Entry && kind != Module)
        return;
    registryTree->setCurrentItem(item);
    showEntryDetails();
    stack->setCurrentWidget(propertiesPage);
}


void JavaCardView::showEntryDetails()
{
    detailsTree->clear();
    const QTreeWidgetItem *current = registryTree->currentItem();
    if (!current)
        return;

    auto row = [this](QTreeWidgetItem *parent, const QString &name, const QString &value) {
        auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(detailsTree);
        item->setText(0, name);
        item->setText(1, value);
        return item;
    };
    auto labelled = [this](const QByteArray &aid) {
        for (const GpRegistryEntry &e : content.registry) {
            if (e.aid == aid && !e.label.isEmpty())
                return QString("%1  %2").arg(aidText(aid), e.label);
        }
        return aidText(aid);
    };

    const int kind = current->data(0, KindRole).toInt();
    if (kind == Group) {
        row(nullptr, "Content", QString("%1 Executable Load Files").arg(current->childCount()));
        return;
    }

    const GpRegistryEntry *e = currentEntry();
    if (!e)
        return;

    if (kind == Module) {
        row(nullptr, "AID", aidText(current->data(0, AidRole).toByteArray()));
        row(nullptr, "Type", "Executable Module");
        row(nullptr, "Executable Load File", labelled(e->aid));
        return;
    }

    row(nullptr, "AID", aidText(e->aid));
    if (!e->label.isEmpty())
        row(nullptr, "Name", e->label);
    row(nullptr, "Type", kindText(e->kind));
    row(nullptr, "Life cycle state", QString("%1 (%2)")
                                         .arg(GpCoding::lifeCycleText(e->codingKind(), e->lifeCycle), hexValue(e->lifeCycle)));

    if (e->kind != GpRegistryEntry::Kind::ExecutableLoadFile) {
        QTreeWidgetItem *p = row(nullptr, "Privileges", Utility::byteArrayToHexString(e->privileges));
        for (const QString &name : GpCoding::privilegeNames(e->privileges))
            row(p, QString(), name);
        p->setExpanded(true);
    }
    if (!e->loadFileAid.isEmpty())
        row(nullptr, "Executable Load File", labelled(e->loadFileAid));
    if (!e->domainAid.isEmpty())
        row(nullptr, "Associated Security Domain", labelled(e->domainAid));
    if (e->kind == GpRegistryEntry::Kind::ExecutableLoadFile) {
        if (e->version.size() == 2)
            row(nullptr, "Version", QString("%1.%2").arg(quint8(e->version[0])).arg(quint8(e->version[1])));
        if (!e->moduleAids.isEmpty()) {
            QTreeWidgetItem *m = row(nullptr, "Executable Modules", QString::number(e->moduleAids.size()));
            for (const QByteArray &module : e->moduleAids)
                row(m, QString(), labelled(module));
            m->setExpanded(true);
        }
    }
}


void JavaCardView::updateActions()
{
    updateChannelUi();
    emit stateChanged();
    const GpRegistryEntry *entry = currentEntry();
    const bool entrySelected = entry && registryTree->currentItem()->data(0, KindRole).toInt() == Entry;
    const QTreeWidgetItem *key = keyTree->currentItem();
    const bool keySelected = key && key->data(0, KindRole).toInt() == Entry;

    detailsToggle->setEnabled(authenticated);
    buttonSendApdu->setEnabled(connected);
    buttonSendBytes->setEnabled(connected);
    apduPanel->setSendEnabled(connected);
    bytesPanel->setSendEnabled(connected);
    buttonAuthenticate->setEnabled(connected);
    updatePropertyActions();
    buttonRefresh->setEnabled(authenticated);
    buttonLoad->setEnabled(authenticated);
    buttonInstall->setEnabled(authenticated);
    buttonDelete->setEnabled(authenticated && entrySelected && entry->kind != GpRegistryEntry::Kind::IssuerSecurityDomain);
    buttonSetStatus->setEnabled(authenticated && entrySelected && entry->kind != GpRegistryEntry::Kind::ExecutableLoadFile);
    buttonPutKey->setEnabled(authenticated);
    buttonDeleteKey->setEnabled(authenticated && keySelected);
}


void JavaCardView::setDetailsVisible(bool visible)
{
    detailsText->setVisible(visible && authenticated);
    detailsToggle->setArrowType(!authenticated ? Qt::NoArrow : visible ? Qt::DownArrow : Qt::RightArrow);
}


// Simulated values: a deterministic stand-in for what the real handshake would produce
void JavaCardView::showSecureChannelDetails()
{
    auto derive = [this](const char *label, int bytes) {
        const QByteArray seed = QByteArray(label) + QByteArray(1, char(auth.securityLevel)) + auth.hostChallenge
                                + QByteArray(1, char(int(auth.protocol)));
        return Utility::byteArrayToHexString(QCryptographicHash::hash(seed, QCryptographicHash::Sha256).left(bytes));
    };
    const bool scp03 = auth.protocol == GpAuthParams::Protocol::Scp03;
    const bool scp11 = auth.isScp11();
    const int keyBytes = 16;

    QStringList lines;
    lines << QString("Protocol:          %1").arg(GpAuthParams::protocolText(auth.protocol));
    lines << QString("Security level:    %1 (%2)").arg(hexValue(auth.securityLevel), GpCoding::securityLevelText(auth.securityLevel));
    lines << QString("Security Domain:   %1").arg(auth.selectFirst ? aidText(auth.targetAid) : QString("selected application"));
    lines << QString("Key version / id:  %1 / %2").arg(auth.keyVersion ? hexValue(auth.keyVersion) : QString("first"),
                                                       hexValue(auth.keyId));
    lines << QString("Host challenge:    %1").arg(auth.hostChallenge.isEmpty() ? derive("host", 8)
                                                                               : Utility::byteArrayToHexString(auth.hostChallenge));
    lines << QString("Card challenge:    %1").arg(derive("card", 8));
    lines << QString("Host cryptogram:   %1").arg(derive("hostc", 8));
    lines << QString("Card cryptogram:   %1").arg(derive("cardc", 8));
    if (scp11) {
        lines << QString("Receipt:           %1").arg(derive("receipt", 16));
        lines << QString("S-ENC:             %1").arg(derive("senc", keyBytes));
        lines << QString("S-MAC:             %1").arg(derive("smac", keyBytes));
        lines << QString("S-RMAC:            %1").arg(derive("srmac", keyBytes));
        lines << QString("S-DEK:             %1").arg(derive("sdek", keyBytes));
    } else {
        if (auth.protocol == GpAuthParams::Protocol::Scp02)
            lines << QString("Sequence counter:  %1").arg(derive("seq", 2));
        lines << QString("S-ENC:             %1").arg(derive("senc", keyBytes));
        lines << QString("S-MAC:             %1").arg(derive("smac", keyBytes));
        if (scp03)
            lines << QString("S-RMAC:            %1").arg(derive("srmac", keyBytes));
        lines << QString("S-DEK:             %1").arg(derive("sdek", keyBytes));
    }
    lines << "";
    lines << "(simulated, nothing was exchanged with the card)";
    detailsText->setPlainText(lines.join('\n'));
    setDetailsVisible(detailsToggle->isChecked());
}


void JavaCardView::updatePropertyActions()
{
    const GpRegistryEntry *entry = currentEntry();
    const bool module = registryTree->currentItem() && registryTree->currentItem()->data(0, KindRole).toInt() == Module;
    const bool usable = authenticated && entry;
    const bool loadFile = entry && entry->kind == GpRegistryEntry::Kind::ExecutableLoadFile;

    buttonPropInstall->setEnabled(usable && loadFile);
    buttonPropSetStatus->setEnabled(usable && !module && !loadFile);
    buttonPropDelete->setEnabled(usable && !module && entry->kind != GpRegistryEntry::Kind::IssuerSecurityDomain);
    buttonPropRefresh->setEnabled(authenticated);
}
