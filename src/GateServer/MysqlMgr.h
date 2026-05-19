#include "const.h"
#include "MysqlDao.h"
class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr();
    //查询用户名或邮箱是否存在，若不存在，注册用户信息
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
    //判断用户名和邮箱是否匹配
    bool CheckEmail(const std::string& name, const std::string& email);
    //更新用户密码
    bool UpdatePwd(const std::string& email, const std::string& newpwd);
    //检验邮箱对应密码是否正确，若正确，获取userInfo
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
private:
    MysqlMgr();
    MysqlDao  _dao;
};