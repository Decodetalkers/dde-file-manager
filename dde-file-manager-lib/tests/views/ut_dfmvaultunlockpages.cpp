#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QTest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <DToolTip>
#include <DPasswordEdit>

#include "../stub-ext/stubext.h"

#define private public
#include "views/dfmvaultunlockpages.h"

DWIDGET_USE_NAMESPACE
namespace  {
    class TestDFMVaultUnlockPages: public testing::Test
    {
    public:
        DFMVaultUnlockPages* m_view;

        virtual void SetUp() override
        {
            m_view = DFMVaultUnlockPages::instance();
            m_view->show();
            std::cout << "start TestDFMVaultUnlockPages" << std::endl;
        }

        virtual void TearDown() override
        {
            m_view->close();
            std::cout << "end TestDFMVaultUnlockPages" << std::endl;
        }
    };
}


TEST_F(TestDFMVaultUnlockPages, tst_onButtonClicked)
{
    m_view->onButtonClicked(1);
    m_view->onButtonClicked(0);
}

TEST_F(TestDFMVaultUnlockPages, tst_enter_password)
{
    m_view->m_passwordEdit->setText("123");
    EXPECT_TRUE(m_view->getButton(1)->isEnabled());

    m_view->m_passwordEdit->setText("");
    EXPECT_FALSE(m_view->getButton(1)->isEnabled());
}

TEST_F(TestDFMVaultUnlockPages, tst_onVaultUlocked)
{
    m_view->m_bUnlockByPwd = true;
    EXPECT_NO_FATAL_FAILURE(m_view->onVaultUlocked(1));
    EXPECT_NO_FATAL_FAILURE(m_view->onVaultUlocked(0));

    // replace DDialog::exec
    int (*st_exec)() = []()->int{
        // do nothing.
        return DDialog::Accepted;
    };
    stub_ext::StubExt stub;
    stub.set(VADDR(DDialog, exec), st_exec);

    m_view->m_bUnlockByPwd = true;
    EXPECT_NO_FATAL_FAILURE(m_view->onVaultUlocked(2));
}

TEST_F(TestDFMVaultUnlockPages, tst_tipsButtonClicked)
{
    QTest::mouseClick(m_view->m_tipsButton, Qt::LeftButton);
}