#include "dcustomactionparser.h"

#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QFileSystemWatcher>

using namespace DCustomActionDefines;

DCustomActionParser::DCustomActionParser(QObject *parent) : QObject(parent)
{
    m_fileWatcher = new QFileSystemWatcher;
    m_fileWatcher->addPath(kCustomMenuPath);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, [this]{
        m_actionEntry.clear();
        loadDir(kCustomMenuPath);
    });
    initHash();
    loadDir(kCustomMenuPath);
    //暂时不考虑效率，todo后续优化考虑开线程处理此loadDir
}

DCustomActionParser::~DCustomActionParser()
{
    if (m_fileWatcher) {
        m_fileWatcher->deleteLater();
        m_fileWatcher = nullptr;
    }
}



/*!
    根据给定的文件夹路径\a dirPath 遍历解析该文件夹下的.conf文件,
    返回值 bool* 为是否成功遍历文件夹。
*/
bool DCustomActionParser::loadDir(const QString &dirPath)
{
    if (dirPath.isEmpty())
        return false;
    m_topActionCount = 0;
    QDir dir(dirPath);
    if (!dir.exists())
        return false;
    m_actionEntry.clear();
    //以时间先后遍历
    for (const QFileInfo &actionFileInfo : dir.entryInfoList({"*.conf"}, QDir::Files, QDir::Time)) {

        //解析文件字段
        QSettings actionSetting(actionFileInfo.filePath(), QSettings::IniFormat);
        actionSetting.setIniCodec("UTF-8");
        parseFile(actionSetting);
    }
    return true;
}

/*!
    返回值QList<DCustomActionEntry>，返回加载解析的菜单项
*/
QList<DCustomActionEntry> DCustomActionParser::getActionFiles()
{
    return m_actionEntry;
}

/*!
    根据传入的\a actionSetting 解析菜单项，返回返回值为解析成功与否，关键字段缺失会被断定未无效文件，归于失败
*/
bool DCustomActionParser::parseFile(QSettings &actionSetting)
{
    //基本信息，版本，选中类型,且选中类型无明确说明则认为是无效的配置文件
    FileBasicInfos basicInfos;
    bool prefixExists = actionSetting.childGroups().contains(kMenuPrefix);
    if (!prefixExists) { //关键入口信息没有，认为是无效的配置文件
        return false;
    }
    if (!actionFileInfos(basicInfos, actionSetting))
        return false;//关键信息无效则

    auto actions = getValue(actionSetting, kMenuPrefix, kActionGroups).toString().trimmed();
    if (actions.isEmpty())
        return false; //无一级菜单,无效文件

    auto actStr = getValue(actionSetting, kMenuPrefix, kActionGroups);
    auto actList = actStr.toString().trimmed().split(":", QString::SkipEmptyParts);
    m_hierarchyNum = 1;
    for(auto &once : actList) {
        if (m_topActionCount == kCustomMaxNumOne) //一级数量限制
            break;
        QList<DCustomActionData> childrenActions;//这个实际上一级时没用
        QString targetGroup = QString("%1 %2").arg(kActionPrefix).arg(once);
        parseFile(childrenActions, actionSetting, targetGroup, basicInfos, true);
        m_topActionCount++;
    }
    return true;
}

/*!
    该方法用于递归解析对应组下的菜单信息，\a childrenActions 获取菜单项，\a actionSetting 为解析对象，\a group 为待解析分组，
\a basicInfos 是为一级菜单项准备的基本信息，\a isTop 表示当前解析的是否是一级菜单
*/
void DCustomActionParser::parseFile(QList<DCustomActionData> &childrenActions, QSettings &actionSetting, const QString &group, const FileBasicInfos &basicInfos, bool isTop)
{
    m_hierarchyNum++;
    if (4 < m_hierarchyNum) //超过三级不解（待与产品沟通，是否需要）
        return;

    DCustomActionData actData;
    //暂时用localname 和name,方式有些不确定，oem和之前的自定义右键是localName，打开方式又好像是genaricName
    //后续确认优化
    QString localName = QString("Name[%1]").arg(QLocale::system().name());
    QString name = getValue(actionSetting, group, localName).toString().trimmed();
    if (name.isEmpty()) {
        name =  getValue(actionSetting, group, kActionName).toString();
        if (name.isEmpty())
            return; //无name无action
    }
    actData.m_name = name;
    actionNameDynamicArg(actData);

    //pos
    actData.m_position =  getValue(actionSetting, group, kActionPos).toInt();

    //separator
    QString separator = getValue(actionSetting, group, kActionSeparator).toString().trimmed();
    actData.m_separator = m_separtor.value(separator, None);

    //actions 父子action级联与动作

    //actions 父级级联与动作
    QString actions = getValue(actionSetting, group, kActionGroups).toString().trimmed();
    if (actions.isEmpty()) {
        //无级联检查是否有动作
        QString command = getValue(actionSetting, group, kActionCmd).toString().trimmed();
        if (command.isEmpty())
            return; //无动作无子级
        actData.m_command = command;
        execDynamicArg(actData);
    }
    else {
        //add 子菜单项，父级有子菜单，则忽略动作，即便子菜单无一有效，后续也不再添加动作
        QList<DCustomActionData> tpChildrenActions;
        auto actStr = getValue(actionSetting, group, kActionGroups);
        auto actList = actStr.toString().trimmed().split(":", QString::SkipEmptyParts);

        int actCount = 0;
        for(auto &once : actList) {
            actCount++;
            QString targetGroup = QString("%1 %2").arg(kActionPrefix).arg(once);
            parseFile(tpChildrenActions, actionSetting, targetGroup, basicInfos);
            m_hierarchyNum--;
            if (2 == m_hierarchyNum && actCount == kCustomMaxNumTwo) //二级数量限制
                break;
            if (3 == m_hierarchyNum && actCount == kCustomMaxNumThree) //三级数量限制
                break;
        }
        if (0 == tpChildrenActions.size())
            return; //作为无动作无子级，不再为其添加已有动作
        actData.m_childrenActions = tpChildrenActions;
    }

    if (isTop) {
        DCustomActionEntry tpEntry;

        //支持类型combo
        auto comboStr = getValue(actionSetting, group, kConfCombo).toString().trimmed();
        if (comboStr.isEmpty()) {
            return;//无支持选中类型默认该一级无效
        }
        else {
            QStringList comboList = comboStr.split(":", QString::SkipEmptyParts);
            ComboTypes target;
            for (auto temp : comboList) {
                auto tp = temp.trimmed();
                if (m_combos.contains(tp))
                    target = target | m_combos.value(temp);
            }
            tpEntry.m_fileCombo = target;
        }
        //todo支持的文件类型(mimeTypes)，目前无需求暂不判断

        //comboPos
        if (comboPosForTopAction(actionSetting, group, actData))
            return;//没有指明该一级菜单项支持的类型，自动作为无效废弃项
        tpEntry.m_package = basicInfos.m_package;
        tpEntry.m_version = basicInfos.m_version;
        tpEntry.m_comment = basicInfos.m_comment;
        tpEntry.m_data = actData;
        m_actionEntry.append(tpEntry);
    }
    else {
        childrenActions.append(actData);
    }
}

/*!
    初始化一些判断内容
*/
void DCustomActionParser::initHash()
{
    m_combos.insert("SingleFile", ComboType::SingleFile);
    m_combos.insert("SingleDir", ComboType::SingleDir);
    m_combos.insert("MultiFiles", ComboType::MultiFiles);
    m_combos.insert("MultiDirs", ComboType::MultiDirs);
    m_combos.insert("FileAndDir", ComboType::FileAndDir);
    m_combos.insert("BlankSpace", ComboType::BlankSpace);

    m_separtor.insert("None", Separator::None);
    m_separtor.insert("Top", Separator::Top);
    m_separtor.insert("Both", Separator::Both);
    m_separtor.insert("Bottom", Separator::Bottom);

    //name参数类型仅支持：DirName BaseName FileName
    m_actionNameArg.insert(kStrActionArg[DirName], ActionArg::DirName);       //%d
    m_actionNameArg.insert(kStrActionArg[BaseName], ActionArg::BaseName);     //%b
    m_actionNameArg.insert(kStrActionArg[FileName], ActionArg::FileName);     //"%a",

    //cmd参数类型只支持：DirPath FilePath FilePaths UrlPath UrlPaths
    m_actionExecArg.insert(kStrActionArg[DirPath], ActionArg::DirPath);       //"%p"
    m_actionExecArg.insert(kStrActionArg[FilePath], ActionArg::FilePath);     //"%f"
    m_actionExecArg.insert(kStrActionArg[FilePaths], ActionArg::FilePaths);   //"%F"
    m_actionExecArg.insert(kStrActionArg[UrlPath], ActionArg::UrlPath);       //"%u"
    m_actionExecArg.insert(kStrActionArg[UrlPaths], ActionArg::UrlPaths);     //"%U"
}

/*!
    获取配置文件对应组下的对应字段信息，\a actionSetting 是解析对象，\a group是待解析的组， \a key是待解析字段
*/
QVariant DCustomActionParser::getValue(QSettings &actionSetting, const QString &group, const QString &key)
{
    actionSetting.beginGroup(group);
    auto temp = actionSetting.value(key, QVariant());
    actionSetting.endGroup();
    return temp;
}

/*!
    获取配置文件的一些基本信息，\a actionSetting获取解析的基本信息，\a actionSetting 是解析对象，
    返回值表示该文件有效与否，基本的关键信息没有会被判定成无效的配置文件
*/
bool DCustomActionParser::actionFileInfos(FileBasicInfos &basicInfo, QSettings &actionSetting)
{
    //基本信息
    //文件名
    basicInfo.m_package = actionSetting.fileName();

    //签名
    basicInfo.m_sign = getValue(actionSetting, kMenuPrefix, kConfSign).toString().trimmed();

    //版本
    basicInfo.m_version = getValue(actionSetting, kMenuPrefix, kConfFileVersion).toString().trimmed();
    if (basicInfo.m_version.isEmpty())
        return false;

    //描述
    basicInfo.m_comment = getValue(actionSetting, kMenuPrefix, kConfComment).toString().trimmed();
    return true;
}

/*!
    菜单项名字参数动态获取
*/
void DCustomActionParser::actionNameDynamicArg(DCustomActionData &act)
{
    //name参数类型仅支持：DirName BaseName FileName
    int firstValidIndex = act.m_name.indexOf("%");
    auto cnt = act.m_name.length() - 1;
    if (0 == cnt || 0 > firstValidIndex) {
        act.m_nameArg = NoneArg;
        return;
    }

    while (cnt > firstValidIndex) {
        auto tgStr = act.m_name.mid(firstValidIndex, 2);
        auto tempValue = m_actionNameArg.value(tgStr, NoneArg);
        if (NoneArg != tempValue) {
            act.m_nameArg = tempValue;
            break;
        }
        firstValidIndex = act.m_name.indexOf("%", firstValidIndex + 1);
        if (-1 == firstValidIndex)
            break;
    }
}

/*!
    菜单项执行参数动态获取
*/
void DCustomActionParser::execDynamicArg(DCustomActionData &act)
{
    //cmd参数类型只支持：DirPath FilePath FilePaths UrlPath UrlPaths
    int firstValidIndex = act.m_command.indexOf("%");
    auto cnt = act.m_command.length() - 1;
    if (0 == cnt || 0 > firstValidIndex) {
        act.m_cmdArg = NoneArg;
        return;
    }

    while (cnt > firstValidIndex) {
        auto tgStr = act.m_command.mid(firstValidIndex, 2);
        auto tempValue = m_actionExecArg.value(tgStr, NoneArg);
        if (NoneArg != tempValue) {
            act.m_cmdArg = tempValue;
            break;
        }
        firstValidIndex = act.m_name.indexOf("%", firstValidIndex + 1);
        if (-1 == firstValidIndex)
            break;
    }
}

/*!
    菜单项对应位置
*/
bool DCustomActionParser::comboPosForTopAction(QSettings &actionSetting, const QString &group, DCustomActionData &act)
{
    //能到这一步说明这个文件的有效性已经验证了
    auto comboStr = getValue(actionSetting, group, kConfCombo).toString().trimmed();
    QStringList comboList = comboStr.split(":", QString::SkipEmptyParts);

    QString cPos;
    for (auto temp : comboList) {
        cPos = QString("%1-%2").arg(kActionPos, temp.trimmed());
        auto ret = getValue(actionSetting, group, cPos);    //取出对应选中类型的pos
        if (m_combos.contains(temp)) {
            int pos = act.m_position;
            if (ret.isValid())
                pos = ret.toInt();
            act.m_comboPos.insert(m_combos.value(temp), pos);
        }
        else {
            return false;   //未指明一级菜单支持的选中类型，做无效处理
        }
    }
    return true;
}
