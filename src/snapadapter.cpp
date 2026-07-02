#include "snapadapter.h"

#include <QHash>

using yas::CliAction;
using yas::CliCommand;
using yas::Package;

// Snap adapter. v1 wraps the snap CLI with pkexec for mutations; migrating
// reads to the snapd REST API (/run/snapd.socket, JSON) is TODO etapa 5.19.
namespace {

const QString kSnap = QStringLiteral("snap");

CliCommand root(QStringList args)
{
    return {QStringLiteral("pkexec"), QStringList{kSnap} + args};
}

// snap tables are whitespace-aligned; first line is the header. Cells are
// sliced by header column start positions (summaries contain spaces).
struct Col { QString title; qsizetype start; qsizetype end; };

QList<QHash<QString, QString>> parseSnapTable(const QString &stdOut,
                                              const QStringList &columnTitles)
{
    QList<QHash<QString, QString>> rows;
    const QStringList lines = stdOut.split(QLatin1Char('\n'));

    QList<Col> columns;
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty())
            continue;
        if (columns.isEmpty()) {
            for (const QString &title : columnTitles) {
                const qsizetype pos = line.indexOf(title);
                if (pos >= 0)
                    columns.append({title.toLower(), pos, -1});
            }
            if (columns.isEmpty())
                return rows; // not a table we recognize
            std::sort(columns.begin(), columns.end(),
                      [](const Col &a, const Col &b) { return a.start < b.start; });
            for (qsizetype i = 0; i < columns.size(); ++i)
                columns[i].end = (i + 1 < columns.size()) ? columns[i + 1].start : -1;
            continue;
        }
        QHash<QString, QString> row;
        for (const Col &c : columns) {
            const QString cell = (c.end < 0) ? line.mid(c.start)
                                             : line.mid(c.start, c.end - c.start);
            row.insert(c.title, cell.trimmed());
        }
        if (!row.value(QStringLiteral("name")).isEmpty())
            rows.append(row);
    }
    return rows;
}

} // namespace

QString SnapAdapter::displayName() const { return QStringLiteral("Snapcraft"); }
QString SnapAdapter::cliProgram() const { return kSnap; }
QStringList SnapAdapter::cliSearchPaths() const
{
    return {QStringLiteral("/usr/bin"), QStringLiteral("/snap/bin")};
}
QStringList SnapAdapter::cliVersionArguments() const { return {QStringLiteral("version")}; }

CliCommand SnapAdapter::searchCommand(const QString &query) const
{
    return {kSnap, {QStringLiteral("find"), query}};
}

CliCommand SnapAdapter::infoCommand(const QString &packageId, const QString &) const
{
    return {kSnap, {QStringLiteral("info"), packageId}};
}

CliCommand SnapAdapter::listInstalledCommand() const
{
    return {kSnap, {QStringLiteral("list")}};
}

CliCommand SnapAdapter::listOutdatedCommand() const
{
    return {kSnap, {QStringLiteral("refresh"), QStringLiteral("--list")}};
}

CliCommand SnapAdapter::installCommand(const QString &packageId, const QString &) const
{
    return root({QStringLiteral("install"), packageId});
}

CliCommand SnapAdapter::uninstallCommand(const QString &packageId, const QString &) const
{
    return root({QStringLiteral("remove"), packageId});
}

CliCommand SnapAdapter::upgradeCommand(const QString &packageId, const QString &) const
{
    return root({QStringLiteral("refresh"), packageId});
}

CliCommand SnapAdapter::upgradeAllCommand() const
{
    return root({QStringLiteral("refresh")});
}

CliCommand SnapAdapter::pinCommand(const QString &packageId, const QString &) const
{
    return root({QStringLiteral("refresh"), QStringLiteral("--hold"), packageId});
}

CliCommand SnapAdapter::unpinCommand(const QString &packageId, const QString &) const
{
    return root({QStringLiteral("refresh"), QStringLiteral("--unhold"), packageId});
}

QList<Package> SnapAdapter::parseSearch(const QString &stdOut) const
{
    // Name  Version  Publisher  Notes  Summary
    QList<Package> result;
    const auto rows = parseSnapTable(
        stdOut, {QStringLiteral("Name"), QStringLiteral("Version"),
                 QStringLiteral("Publisher"), QStringLiteral("Notes"),
                 QStringLiteral("Summary")});
    for (const auto &row : rows) {
        Package p;
        p.id = row.value(QStringLiteral("name"));
        p.name = p.id;
        p.version = row.value(QStringLiteral("version"));
        p.description = row.value(QStringLiteral("summary"));
        p.source = row.value(QStringLiteral("publisher"));
        p.kind = QStringLiteral("snap");
        result.append(p);
    }
    return result;
}

QList<Package> SnapAdapter::parseInfo(const QString &stdOut) const
{
    // yaml-ish "key: value"; "installed:" carries the local version.
    Package p;
    const QStringList lines = stdOut.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const qsizetype colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        const QString key = line.left(colon).trimmed();
        const QString value = line.mid(colon + 1).trimmed();
        if (key == QStringLiteral("name")) { p.id = value; p.name = value; }
        else if (key == QStringLiteral("summary")) p.description = value;
        else if (key == QStringLiteral("publisher")) p.source = value;
        else if (key == QStringLiteral("store-url")) p.homepage = value;
        else if (key == QStringLiteral("installed"))
            p.installedVersion = value.section(QLatin1Char(' '), 0, 0);
        else if (key == QStringLiteral("latest/stable") && p.version.isEmpty())
            p.version = value.section(QLatin1Char(' '), 0, 0);
    }
    if (p.id.isEmpty())
        return {};
    if (p.version.isEmpty())
        p.version = p.installedVersion;
    p.kind = QStringLiteral("snap");
    return {p};
}

QList<Package> SnapAdapter::parseInstalled(const QString &stdOut) const
{
    // Name  Version  Rev  Tracking  Publisher  Notes
    QList<Package> result;
    const auto rows = parseSnapTable(
        stdOut, {QStringLiteral("Name"), QStringLiteral("Version"), QStringLiteral("Rev"),
                 QStringLiteral("Tracking"), QStringLiteral("Publisher"),
                 QStringLiteral("Notes")});
    for (const auto &row : rows) {
        Package p;
        p.id = row.value(QStringLiteral("name"));
        p.name = p.id;
        p.installedVersion = row.value(QStringLiteral("version"));
        p.version = p.installedVersion;
        p.source = row.value(QStringLiteral("tracking"));
        p.kind = QStringLiteral("snap");
        p.pinned = row.value(QStringLiteral("notes")).contains(QStringLiteral("held"));
        result.append(p);
    }
    return result;
}

QList<Package> SnapAdapter::parseOutdated(const QString &stdOut) const
{
    // refresh --list: Name  Version  Rev  Size  Publisher  Notes
    // (version shown is the NEW one; snap does not print the installed one)
    QList<Package> result;
    const auto rows = parseSnapTable(
        stdOut, {QStringLiteral("Name"), QStringLiteral("Version"), QStringLiteral("Rev"),
                 QStringLiteral("Size"), QStringLiteral("Publisher"),
                 QStringLiteral("Notes")});
    for (const auto &row : rows) {
        Package p;
        p.id = row.value(QStringLiteral("name"));
        p.name = p.id;
        p.version = row.value(QStringLiteral("version"));
        p.installedVersion = QStringLiteral("?"); // not reported by the CLI
        p.kind = QStringLiteral("snap");
        result.append(p);
    }
    return result;
}

QList<CliAction> SnapAdapter::actionCatalog() const
{
    return {
        {QStringLiteral("changes"), tr("Recent changes"),
         tr("Show the latest snap system changes and their status"),
         {kSnap, {QStringLiteral("changes")}}, false, false, false},
        {QStringLiteral("services"), tr("List services"),
         tr("Status of services provided by installed snaps"),
         {kSnap, {QStringLiteral("services")}}, false, false, false},
        {QStringLiteral("refresh-time"), tr("Refresh schedule"),
         tr("When the next automatic refresh will run"),
         {kSnap, {QStringLiteral("refresh"), QStringLiteral("--time")}}, false, false, false},
        {QStringLiteral("revert"), tr("Revert to previous revision"),
         tr("Roll a snap back to its previously installed revision"),
         root({QStringLiteral("revert")}), true, true, true},
        {QStringLiteral("connections"), tr("Show connections"),
         tr("Interface connections (permissions) of a snap"),
         {kSnap, {QStringLiteral("connections")}}, true, false, false},
        {QStringLiteral("version"), tr("Snapd version"),
         tr("Versions of snap, snapd and the running system"),
         {kSnap, {QStringLiteral("version")}}, false, false, false},
    };
}
