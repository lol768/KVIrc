//=============================================================================
//
//   File : libkvitip.cpp
//   Creation date : Thu May 10 2001 13:50:11 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2001-2008 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#include "libkvitip.h"

#include "kvi_module.h"
#include "kvi_locale.h"
#include "kvi_app.h"
#include "kvi_iconmanager.h"
#include "kvi_options.h"
#include "kvi_fileutils.h"

#include <QPushButton>
#include <QFont>
#include <QPainter>
#include <QDesktopWidget>
#include <QCloseEvent>

KviTipWindow * g_pTipWindow = 0;

KviTipFrame::KviTipFrame(QWidget * par)
: QFrame(par)
{
	QString buffer;
	m_pLabel1 = new QLabel(this);
	m_pLabel2 = new QLabel(this);
	g_pApp->findImage(buffer,"kvi_tip.png");
	m_pLabel1->setPixmap(buffer);
	setStyleSheet("QFrame { background: black; }");
	m_pLabel1->setStyleSheet("QLabel { background: black; }");
	m_pLabel2->setStyleSheet("QLabel { background: black; color: white; }");
	m_pLabel2->setWordWrap(true);
	m_pLabel2->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	setFrameStyle(QFrame::Sunken | QFrame::WinPanel);
	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_pLabel1,0,0,1,1);
	layout->addWidget(m_pLabel2,0,1,1,1);
	layout->setColumnStretch(1,1);
	setLayout(layout);
}

KviTipFrame::~KviTipFrame()
{
}

void KviTipFrame::setText(const QString &text)
{
	QString szText= "<center>";
	szText += text;
	szText += "</center>";
	m_pLabel2->setText(szText);
	update();
}

KviTipWindow::KviTipWindow()
{
	setObjectName("kvirc_tip_window");
	m_pConfig = 0;

	m_pTipFrame = new KviTipFrame(this);
	QPushButton * pb = new QPushButton("<<",this);
	connect(pb,SIGNAL(clicked()),this,SLOT(prevTip()));

	QPushButton * pb2 = new QPushButton(">>",this);
	connect(pb2,SIGNAL(clicked()),this,SLOT(nextTip()));

	QPushButton * pb3 = new QPushButton(__tr2qs("Close"),this);
	connect(pb3,SIGNAL(clicked()),this,SLOT(close()));
	pb3->setDefault(true);

	m_pShowAtStartupCheck = new QCheckBox(__tr2qs("Show at startup"),this);
	m_pShowAtStartupCheck->setChecked(KVI_OPTION_BOOL(KviOption_boolShowTipAtStartup));

	setWindowIcon(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_IDEA)));

	setWindowTitle(__tr2qs("Did you know..."));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_pTipFrame,0,0,1,5);
	layout->addWidget(m_pShowAtStartupCheck,1,0,1,1);
	layout->addWidget(pb,1,2,1,1);
	layout->addWidget(pb2,1,3,1,1);
	layout->addWidget(pb3,1,4,1,1);
	setLayout(layout);

	pb3->setFocus();

}

KviTipWindow::~KviTipWindow()
{
	KVI_OPTION_BOOL(KviOption_boolShowTipAtStartup) = m_pShowAtStartupCheck->isChecked();
	if(m_pConfig)closeConfig();
}

void KviTipWindow::showEvent(QShowEvent *)
{
	QRect rect = g_pApp->desktop()->screenGeometry(g_pApp->desktop()->primaryScreen());
	move((rect.width() - width())/2,(rect.height() - height())/2);
}

bool KviTipWindow::openConfig(QString filename,bool bEnsureExists)
{
	if(m_pConfig)closeConfig();

	m_szConfigFileName = filename;
//	m_szConfigFileName.cutToLast('/');

	QString buffer;
	g_pApp->getReadOnlyConfigPath(buffer,m_szConfigFileName.toUtf8().data(),KviApp::ConfigPlugins,true);
	debug("Check path %s and file %s",buffer.toUtf8().data(),m_szConfigFileName.toUtf8().data());
	if(bEnsureExists)
	{
		if(!KviFileUtils::fileExists(buffer))return false;
	}

	m_pConfig = new KviConfig(buffer,KviConfig::Read);

	return true;
}

void KviTipWindow::closeConfig()
{
	QString buffer;
	g_pApp->getLocalKvircDirectory(buffer,KviApp::ConfigPlugins,m_szConfigFileName);
	m_pConfig->setSavePath(buffer);
	delete m_pConfig;
	m_pConfig = 0;
}

void KviTipWindow::nextTip()
{
	if(!m_pConfig)
	{
		KviStr szLocale = KviLocale::localeName();
		KviStr szFile;
		szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
		if(!openConfig(szFile.ptr(),true))
		{
			szLocale.cutFromFirst('.');
			szLocale.cutFromFirst('_');
			szLocale.cutFromFirst('@');
			szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
			if(!openConfig(szFile.ptr(),true))
			{
				openConfig("libkvitip.kvc",false);
			}
		}
	}

	unsigned int uNumTips = m_pConfig->readUIntEntry("uNumTips",0);
	unsigned int uCurTip = m_pConfig->readUIntEntry("uCurTip",0);

	uCurTip++;
	if(uCurTip >= uNumTips)uCurTip = 0;

	KviStr tmp(KviStr::Format,"%u",uCurTip);
	QString szTip = m_pConfig->readEntry(tmp.ptr(),__tr2qs("<b>Can't find any tip... :(</b>"));

	//qDebug("REDECODED=%s",szTip.toUtf8().data());

	m_pConfig->writeEntry("uCurTip",uCurTip);

	m_pTipFrame->setText(szTip);
}

void KviTipWindow::prevTip()
{
	if(!m_pConfig)
	{
		KviStr szLocale = KviLocale::localeName();
		KviStr szFile;
		szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
		if(!openConfig(szFile.ptr(),true))
		{
			szLocale.cutFromFirst('.');
			szLocale.cutFromFirst('_');
			szLocale.cutFromFirst('@');
			szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
			if(!openConfig(szFile.ptr(),true))
			{
				openConfig("libkvitip.kvc",false);
			}
		}
	}

	unsigned int uNumTips = m_pConfig->readUIntEntry("uNumTips",0);
	unsigned int uCurTip = m_pConfig->readUIntEntry("uCurTip",0);

	
	if(uCurTip == 0)uCurTip = uNumTips-1;
	else uCurTip--;

	KviStr tmp(KviStr::Format,"%u",uCurTip);
	QString szTip = m_pConfig->readEntry(tmp.ptr(),__tr2qs("<b>Can't find any tip... :(</b>"));

	//qDebug("REDECODED=%s",szTip.toUtf8().data());

	m_pConfig->writeEntry("uCurTip",uCurTip);

	m_pTipFrame->setText(szTip);
}

void KviTipWindow::closeEvent(QCloseEvent *e)
{
	e->ignore();
	delete this;
	g_pTipWindow = 0;
}

/*
	@doc: tip.open
	@type:
		command
	@title:
		tip.open
	@short:
		Opens the "did you know..." tip window
	@syntax:
		tip.open [tip_file_name:string]
	@description:
		Opens the "did you know..." tip window.<br>
		If <tip_file_name> is specified , that tip is used instead of
		the default tips provided with kvirc.<br>
		<tip_file_name> must be a file name with no path and must refer to a
		standard KVIrc configuration file found in the global or local
		KVIrc plugin configuration directory ($KVIrcDir/config/modules).<br>
		Once the window has been opened, the next tip avaiable in the config file is shown.<br>
		This command works even if the tip window is already opened.<br>
*/

static bool tip_kvs_cmd_open(KviKvsModuleCommandCall * c)
{
	QString szTipfilename;
	KVSM_PARAMETERS_BEGIN(c)
		KVSM_PARAMETER("filename",KVS_PT_STRING,KVS_PF_OPTIONAL,szTipfilename)
	KVSM_PARAMETERS_END(c)

	if(!g_pTipWindow)g_pTipWindow = new KviTipWindow();
	if (!szTipfilename.isEmpty())
		g_pTipWindow->openConfig(szTipfilename);
	g_pTipWindow->nextTip();
	g_pTipWindow->show();
	return true;
}

static bool tip_module_init(KviModule *m)
{
	KVSM_REGISTER_SIMPLE_COMMAND(m,"open",tip_kvs_cmd_open);
	return true;
}

static bool tip_module_cleanup(KviModule *)
{
	if(g_pTipWindow)g_pTipWindow->close();
	return true;
}

static bool tip_module_can_unload(KviModule *)
{
	return (g_pTipWindow == 0);
}

KVIRC_MODULE(
	"Tip",                                              // module name
	"4.0.0",                                                // module version
	"Copyright (C) 2000 Szymon Stefanek (pragma at kvirc dot net)", // author & (C)
	"\"Did you know...\" tip",
	tip_module_init,
	tip_module_can_unload,
	0,
	tip_module_cleanup,
	0
)

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "libkvitip.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES
