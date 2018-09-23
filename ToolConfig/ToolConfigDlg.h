
// ToolConfigDlg.h: 头文件
//

#pragma once

#define DEF_CONFIGGILE_NAME "\\BiliConfig.ini"
static int DEF_OPTNUM = 8;

// CToolConfigDlg 对话框
class CToolConfigDlg : public CDialogEx
{
// 构造
public:
	CToolConfigDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TOOLCONFIG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_userlist;
	afx_msg void OnUserSelchange();
	afx_msg void OnImportConfig();
	afx_msg void OnUpdateConfig();

private:
	char _cfgfile[MAX_PATH];
	int _ReadConfSummary();
	int _WriteConfSummary();
	int _ReadConfUser(int index);
	int _WriteConfUser(int index);

};
