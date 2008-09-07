/* Copyright (C) 2007 The SpringLobby Team. All rights reserved. */
//
// Class: Spring
//

#include <wx/file.h>
#include <wx/intl.h>
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <fstream>
#include <clocale>

#include "spring.h"
#include "springprocess.h"
#include "ui.h"
#include "utils.h"
#include "settings.h"
#include "userlist.h"
#include "battle.h"
#include "singleplayerbattle.h"
#include "user.h"
#include "iunitsync.h"
#include "nonportable.h"
#include "tdfcontainer.h"
#ifndef NO_TORRENT_SYSTEM
#include "torrentwrapper.h"
#endif

BEGIN_EVENT_TABLE( Spring, wxEvtHandler )

    EVT_COMMAND ( PROC_SPRING, wxEVT_SPRING_EXIT, Spring::OnTerminated )

END_EVENT_TABLE();

#define FIRST_UDP_SOURCEPORT 8300


Spring::Spring( Ui& ui ) :
  m_ui(ui),
  m_process(0),
  m_wx_process(0),
  m_running(false)
{ }


Spring::~Spring()
{
  if ( m_process != 0 )
    delete m_process;
}


bool Spring::IsRunning()
{
  return m_process != 0;
}


bool Spring::Run( Battle& battle )
{
  if ( m_running ) {
    wxLogError( _T("Spring already running!") );
    return false;
  }

  wxString path = sett().GetSpringDir() + wxFileName::GetPathSeparator();

  wxLogMessage( _T("Path to script: %sscript.txt"), path.c_str() );

  try {

    if ( !wxFile::Access( path +  _T("script.txt"), wxFile::write ) ) {
      wxLogError( _T("Access denied to script.txt.") );
    }



    wxFile f( path + _T("script.txt"), wxFile::write );
    f.Write( WriteScriptTxt(battle) );
    f.Close();

  } catch (...) {
    wxLogError( _T("Couldn't write script.txt") );
    return false;
  }

  #ifndef NO_TORRENT_SYSTEM
  wxString CommandForAutomaticTeamSpeak = _T("SCRIPT|") + battle.GetMe().GetNick() + _T("|");
  for ( user_map_t::size_type i = 0; i < battle.GetNumUsers(); i++ )
  {
    CommandForAutomaticTeamSpeak << battle.GetUser(i).GetNick() << _T("|") << u2s( battle.GetUser(i).BattleStatus().ally) << _T("|");
  }
  torrent().SendMessageToCoordinator(CommandForAutomaticTeamSpeak);
  #endif

  wxString cmd =  _T("\"") + sett().GetSpringUsedLoc() + _T("\" \"") + path +  _T("script.txt\"");
  wxLogMessage( _T("cmd: %s"), cmd.c_str() );
  wxSetWorkingDirectory( sett().GetSpringDir() );
  if ( sett().UseOldSpringLaunchMethod() ) {
    if ( m_wx_process == 0 ) m_wx_process = new wxSpringProcess( *this );
    if ( wxExecute( cmd , wxEXEC_ASYNC, m_wx_process ) == 0 ) return false;
  } else {
    if ( m_process == 0 ) m_process = new SpringProcess( *this );
    wxLogMessage( _T("m_process->Create();") );
    m_process->Create();
    wxLogMessage( _T("m_process->SetCommand( cmd );") );
    m_process->SetCommand( cmd );
    wxLogMessage( _T("m_process->Run();") );
    m_process->Run();
  }
  m_running = true;
  wxLogMessage( _T("Done running = true") );
  return true;
}


bool Spring::Run( SinglePlayerBattle& battle )
{
  if ( m_running ) {
    wxLogError( _T("Spring already running!") );
    return false;
  }

  wxString path = wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator();

  try {

    if ( !wxFile::Access( path + _T("script.txt"), wxFile::write ) ) {
      wxLogError( _T("Access denied to script.txt.") );
    }

    wxFile f( path + _T("script.txt"), wxFile::write );
    f.Write( WriteSPScriptTxt(battle) );
    f.Close();

  } catch (...) {
    wxLogError( _T("Couldn't write script.txt") );
    return false;
  }

  wxString cmd =  _T("\"") + sett().GetSpringUsedLoc() + _T("\" \"") + path + _T("script.txt\"");
  wxSetWorkingDirectory( sett().GetSpringDir() );
  if ( sett().UseOldSpringLaunchMethod() ) {
    if ( m_wx_process == 0 ) m_wx_process = new wxSpringProcess( *this );
    if ( wxExecute( cmd , wxEXEC_ASYNC, m_wx_process ) == 0 ) return false;
  } else {
    if ( m_process == 0 ) m_process = new SpringProcess( *this );
    m_process->Create();
    m_process->SetCommand( cmd );
    m_process->Run();
  }

  m_running = true;
  return true;
}


bool Spring::TestSpringBinary()
{
  if ( !wxFileName::FileExists( sett().GetSpringUsedLoc() ) ) return false;
  if ( usync()->GetSpringVersion() != _T("")) return true;
  else return false;
}


void Spring::OnTerminated( wxCommandEvent& event )
{
  wxLogDebugFunc( _T("") );
  m_running = false;
  m_process = 0; // NOTE I'm not sure if this should be deleted or not, according to wx docs it shouldn't.
  m_wx_process = 0;
  m_ui.OnSpringTerminated( true );
}


  struct UserOrder{
    int index;/// user number for GetUser
    int order;/// user order (we'll sort by it)
    bool operator<(UserOrder b) const {/// comparison function for sorting
      return order<b.order;
    }
  };


wxString Spring::WriteScriptTxt( Battle& battle )
{
  wxLogMessage(_T("0 WriteScriptTxt called "));

  wxString ret;

  TDFWriter tdf(ret);

  int  NumTeams=0, MyPlayerNum=-1;

  std::vector<UserOrder> ordered_users;
  std::vector<UserOrder> ordered_bots;
  std::vector<int> TeamConv, AllyConv, AllyRevConv;
  /// AllyRevConv.size() gives number of allies

  wxLogMessage(_T("1 numusers: %d"),battle.GetNumUsers() );

  /// Fill ordered_users and sort it
  for ( user_map_t::size_type i = 0; i < battle.GetNumUsers(); i++ ){
    UserOrder tmp;
    tmp.index=i;
    tmp.order=battle.GetUser(i).BattleStatus().order;
    ordered_users.push_back(tmp);
  }
  std::sort(ordered_users.begin(),ordered_users.end());

  /// process ordered users, convert teams and allys
  for(int i=0;i<int(ordered_users.size());++i){
    wxLogMessage(_T("2 order %d index %d"), ordered_users[i].order, ordered_users[i].index);
    User &user = battle.GetUser(ordered_users[i].index);

    if ( &user == &battle.GetMe() ) MyPlayerNum = i;

    UserBattleStatus status = user.BattleStatus();
    if ( status.spectator ) continue;

    if(status.team>int(TeamConv.size())-1){
      TeamConv.resize(status.team+1,-1);
    }
    if ( TeamConv[status.team] == -1 ) TeamConv[status.team] = NumTeams++;

    if(status.ally>int(AllyConv.size())-1){
      AllyConv.resize(status.ally+1,-1);
    }
    if ( AllyConv[status.ally] == -1 ) {
      AllyConv[status.ally] = AllyRevConv.size();
      AllyRevConv.push_back(status.ally);
    }

  }
  wxLogMessage(_T("3"));

  /// sort and process bots now

  for ( std::list<BattleBot*>::size_type i = 0; i < battle.GetNumBots(); i++ ) {
    UserOrder tmp;
    tmp.index=i;
    tmp.order=battle.GetBot(i)->bs.order;
    ordered_bots.push_back(tmp);
  }
  std::sort(ordered_bots.begin(),ordered_bots.end());

  for(int i=0;i<int(ordered_bots.size());++i){

    BattleBot &bot=*battle.GetBot(ordered_bots[i].index);

    if(bot.bs.ally>int(AllyConv.size())-1){
      AllyConv.resize(bot.bs.ally+1,-1);
    }
    if(AllyConv[bot.bs.ally]==-1){
      AllyConv[bot.bs.ally]=AllyRevConv.size();
      AllyRevConv.push_back(bot.bs.ally);
    }
  }

  ///(dmytry aka dizekat) i don't want to change code below, so these vars for compatibility. If you want, refactor it.

  int NumAllys=AllyRevConv.size();
  int NumBots=ordered_bots.size();


  wxLogMessage(_T("7"));

  //BattleOptions bo = battle.opts();

  // Start generating the script.
  tdf.EnterSection(_T("GAME"));

  tdf.Append(_T("Mapname"),battle.GetHostMapName());
  tdf.Append(_T("GameType"),usync()->GetModArchive(usync()->GetModIndex(battle.GetHostModName())));


  unsigned long uhash;
  battle.LoadMod().hash.ToULong(&uhash);

  tdf.Append(_T("ModHash"),int(uhash));


  wxStringTripleVec optlistEng;
  battle.CustomBattleOptions()->getOptions( &optlistEng, EngineOption );
  for (wxStringTripleVec::iterator it = optlistEng.begin(); it != optlistEng.end(); ++it)
  {
    tdf.Append(it->first,it->second.second);
  }

  if ( battle.IsFounderMe() ){
    tdf.Append(_T("HostIP"),_T("localhost"));
  }else{
    tdf.Append(_T("HostIP"),battle.GetHostIp());
  }

  if ( battle.IsFounderMe() && battle.GetNatType() == NAT_Hole_punching ) {
    tdf.Append(_T("HostPort"),battle.GetMyInternalUdpSourcePort());
  } else {
    tdf.Append(_T("HostPort"),battle.GetHostPort());
  }

  tdf.AppendLineBreak();

  if ( !battle.IsFounderMe() )
  {
    if ( battle.GetNatType() == NAT_Fixed_source_ports )
    {
      tdf.Append(_T("SourcePort"), FIRST_UDP_SOURCEPORT + MyPlayerNum -1);
    }
    else if ( battle.GetNatType() == NAT_Hole_punching )
    {
      tdf.Append(_T("SourcePort"), battle.GetMyInternalUdpSourcePort());
    }
  }

  //s += _T("\n");
  tdf.AppendLineBreak();

  tdf.Append(_T("MyPlayerNum"),MyPlayerNum);
  tdf.Append(_T("NumPlayers"),battle.GetNumUsers());

  tdf.Append(_T("NumTeams"), NumTeams + NumBots);
  tdf.Append(_T("NumAllyTeams"), NumAllys);

  wxLogMessage(_T("8"));
  for ( user_map_t::size_type i = 0; i < battle.GetNumUsers(); i++ ) {
    tdf.EnterSection(_T("PLAYER")+i2s(i));

    tdf.Append(_T("name"),battle.GetUser( ordered_users[i].index ).GetNick());

    tdf.Append(_T("countryCode"),battle.GetUser( ordered_users[i].index ).GetCountry().Lower());
    tdf.Append(_T("Spectator"), battle.GetUser( ordered_users[i].index ).BattleStatus().spectator?1:0);

    if ( !(battle.GetUser( ordered_users[i].index ).BattleStatus().spectator) ) {
      tdf.Append(_T("team"), TeamConv[battle.GetUser( ordered_users[i].index ).BattleStatus().team]);

    }
    tdf.LeaveSection();
  }
  wxLogMessage(_T("9"));

  //s += _T("\n");
  tdf.AppendLineBreak();


  for ( int i = 0; i < NumTeams; i++ ) {
    tdf.EnterSection(_T("TEAM")+i2s(i));

    // Find Team Leader.
    int TeamLeader = -1;

    wxLogMessage(_T("10"));
    for( user_map_t::size_type tlf = 0; tlf < battle.GetNumUsers(); tlf++ ) {/// change: moved check if spectator above use of TeamConv array, coz TeamConv array does not include spectators.

      // Make sure player is not spectator.
      if ( battle.GetUser( ordered_users[tlf].index ).BattleStatus().spectator ) continue;

      // First Player That Is In The Team Is Leader.
      if ( TeamConv[battle.GetUser( ordered_users[tlf].index ).BattleStatus().team] == i ) {



        // Assign as team leader.
        TeamLeader = tlf;
        break;
      }

    }
    wxLogMessage(_T("11"));

    tdf.Append(_T("TeamLeader"),TeamLeader);
    tdf.Append(_T("AllyTeam"),AllyConv[battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().ally]);

    wxString colourstring =
      TowxString( battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().colour.Red()/255.0 ) + _T(' ') +
      TowxString( battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().colour.Green()/255.0 ) + _T(' ') +
      TowxString( battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().colour.Blue()/255.0 );
    tdf.Append(_T("RGBColor"), colourstring);

    wxLogMessage( _T("%d"), battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().side );

    tdf.Append(_T("Side"),usync()->GetSideName( battle.GetHostModName(), battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().side ));
    tdf.Append(_T("Handicap"), battle.GetUser( ordered_users[TeamLeader].index ).BattleStatus().handicap);

    tdf.LeaveSection();
  }
  wxLogMessage( _T("12") );

  for ( int i = 0; i < NumBots; i++ ) {

    tdf.EnterSection(_T("TEAM")+i2s(i + NumTeams));

    BattleBot& bot = *battle.GetBot( ordered_bots[i].index );

    // Find Team Leader.
    int TeamLeader;
    for(TeamLeader=0;TeamLeader<int(ordered_users.size());TeamLeader++){
      if(&battle.GetUser(ordered_users[TeamLeader].index)==&battle.GetUser(bot.owner))goto leader_found;
    }
    TeamLeader=0;
    wxLogMessage( _T("Internal error: team leader not found for bot! Using 0") );
    leader_found:
    /*
    if(battle.GetUser( bot.owner ).BattleStatus().spectator){
      TeamLeader=battle.GetUser( bot.owner ).BattleStatus().team;
    }else{
      TeamLeader = TeamConv[ battle.GetUser( bot.owner ).BattleStatus().team ];
    }*/

    tdf.Append(_T("TeamLeader"),TeamLeader);

    tdf.Append(_T("AllyTeam"),AllyConv[bot.bs.ally]);

    wxString colourstring =
      TowxString( bot.bs.colour.Red()/255.0f ) + _T(' ') +
      TowxString( bot.bs.colour.Green()/255.0f ) + _T(' ') +
      TowxString( bot.bs.colour.Blue()/255.0f );
    tdf.Append(_T("RGBColor"), colourstring);

    tdf.Append(_T("Side"),usync()->GetSideName( battle.GetHostModName(), bot.bs.side ));


    tdf.Append(_T("Handicap"),bot.bs.handicap);
    wxString ai = WX_STRING( bot.aidll );
/*    if ( wxFileName::FileExists( sett().GetSpringDir() + wxFileName::GetPathSeparator() + _T("AI") + wxFileName::GetPathSeparator() + _T("Bot-libs") + wxFileName::GetPathSeparator() + ai + _T(".dll") ) ) {
      ai += _T(".dll");
    } else {
      ai += _T(".so");
    }*/

    //s += ("\t\tAIDLL=AI/Bot-libs/" + ai + ";\n");
    tdf.Append(_T("AIDLL"),ai);
    tdf.LeaveSection();
  }
  wxLogMessage( _T("13") );

  long startpostype;
  battle.CustomBattleOptions()->getSingleValue( _T("startpostype"), EngineOption ).ToLong( &startpostype );
  for ( int i = 0; i < NumAllys; i++ ) {
    tdf.EnterSection(_T("ALLYTEAM")+i2s(i));

    int NumInAlly = 0;    // ???
    //s += wxString::Format( _T("\t\tNumAllies=%d;\n"), NumInAlly );
    tdf.Append(_T("NumAllies"),NumInAlly);

   if ( ( battle.GetStartRect(AllyRevConv[i]).exist ) && (startpostype == ST_Choose) ) {
      BattleStartRect sr = battle.GetStartRect(AllyRevConv[i]);
      if ( sr.exist && !sr.todelete )
      {
          const char* old_locale = std::setlocale(LC_NUMERIC, "C");

          tdf.Append(_T("StartRectLeft"),wxString::Format( _T("%.3f"), sr.left / 200.0 ));
          tdf.Append(_T("StartRectTop"),wxString::Format( _T("%.3f"), sr.top / 200.0 ));
          tdf.Append(_T("StartRectRight"),wxString::Format( _T("%.3f"), sr.right / 200.0 ));
          tdf.Append(_T("StartRectBottom"),wxString::Format( _T("%.3f"), sr.bottom / 200.0 ));

          std::setlocale(LC_NUMERIC, old_locale);
      }
    }

    //s +=  _T("\t}\n");
    tdf.LeaveSection();
  }

  wxLogMessage( _T("14") );

  wxArrayString units = battle.DisabledUnits();

  tdf.Append(_T("NumRestrictions"),units.GetCount());
  tdf.EnterSection(_T("RESTRICT"));
  for ( unsigned int i = 0; i < units.GetCount(); i++) {
    tdf.Append(_T("Unit")+i2s(i),units[i].c_str());
    tdf.Append(_T("Limit")+i2s(i),_T("0"));
    //s += wxString::Format(_T("\t\tUnit%d=%s;\n"), i, units[i].c_str() );
    //s += wxString::Format( _T("\t\tLimit%d=0;\n"), i );
  }
  tdf.LeaveSection();
  wxLogMessage( _T("15") );

  tdf.EnterSection(_T("mapoptions"));

  wxStringTripleVec optlistMap;
  battle.CustomBattleOptions()->getOptions( &optlistMap, MapOption );
  for (wxStringTripleVec::iterator it = optlistMap.begin(); it != optlistMap.end(); ++it)
  {
    tdf.Append(it->first,it->second.second);
  }
  tdf.LeaveSection();

  wxLogMessage( _T("16") );

  tdf.EnterSection(_T("modoptions"));

  wxStringTripleVec optlistMod;
  battle.CustomBattleOptions()->getOptions( &optlistMod, ModOption );
  for (wxStringTripleVec::iterator it = optlistMod.begin(); it != optlistMod.end(); ++it)
  {
    tdf.Append(it->first,it->second.second);
  }
  tdf.LeaveSection();

  wxLogMessage( _T("17") );

  //s += _T("}\n");
  tdf.LeaveSection();

  return ret;

  /// TODO: make TDFWriter handle it all right.
/*
  if ( DOS_TXT ) {
    s.Replace( _T("\n"), _T("\r\n"), true );
  }
  */

  /*

  wxString ds = _T("[Script End]\n\n----------------------[ Debug info ]-----------------------------\nUsers:\n\n");
  for ( user_map_t::size_type i = 0; i < battle.GetNumUsers(); i++ ) {
    User& tmpu = battle.GetUser( i );
    ds += tmpu.GetNick();
    ds += wxString::Format( _T(":\n  team: %d ally: %d spec: %d order: %d side: %d hand: %d sync: %d ready: %d col: %d,%d,%d\n\n"),
      tmpu.BattleStatus().team,
      tmpu.BattleStatus().ally,
      tmpu.BattleStatus().spectator,
      tmpu.BattleStatus().order,
      tmpu.BattleStatus().side,
      tmpu.BattleStatus().handicap,
      tmpu.BattleStatus().sync,
      tmpu.BattleStatus().ready,
      tmpu.BattleStatus().colour.Red(),
      tmpu.BattleStatus().colour.Green(),
      tmpu.BattleStatus().colour.Blue()
    );
  }

  wxLogMessage( _T("18") );
  ds += _T("\n\nBots: \n\n");
  for ( std::list<BattleBot*>::size_type i = 0; i < battle.GetNumBots(); i++ ) {
    BattleBot* bot = battle.GetBot(i);
    if ( bot == 0 ) {
      ds += _T( "NULL" );
      continue;
    }
    ds += bot->name + _T(" (") + bot->owner + _T(")");
    ds += wxString::Format( _T(":\n  team: %d ally: %d spec: %d order: %d side: %d hand: %d sync: %d ready: %d col: %d,%d,%d\n\n"),
      bot->bs.team,
      bot->bs.ally,
      bot->bs.spectator,
      bot->bs.order,
      bot->bs.side,
      bot->bs.handicap,
      bot->bs.sync,
      bot->bs.ready,
      bot->bs.colour.Red(),
      bot->bs.colour.Green(),
      bot->bs.colour.Blue()
    );
  }
  wxLogMessage( _T("19") );

  ds += _T("\n\nPlayerOrder: { ");
  for ( std::vector<UserOrder>::size_type i = 0; i < ordered_users.size(); i++ ) {
    ds += wxString::Format( _T(" %d,"), ordered_users[i].index );
  }
  ds += _T(" }\n\n");

  ds += _T("TeamConv: { ");
  for ( std::vector<int>::size_type i = 0; i < TeamConv.size(); i++ ) {
    ds += wxString::Format( _T(" %d,"), TeamConv[i] );
  }
  ds += _T(" }\n\n");

  ds += _T("AllyConv: { ");
  for ( std::vector<int>::size_type i = 0; i < AllyConv.size(); i++ ) {
    ds += wxString::Format( _T(" %d,"), AllyConv[i] );
  }
  ds += _T(" }\n\n");

  ds += _T("AllyRevConv: { ");
  for ( std::vector<int>::size_type i = 0; i < AllyRevConv.size(); i++ ) {
    ds += wxString::Format( _T(" %d,"), AllyRevConv[i] );
  }
  ds += _T(" }\n\n\n");

  ds += wxString::Format( _T("NumTeams: %d\n\nNumAllys: %d\n\nMyPlayerNum: %d\n\n"), NumTeams, NumAllys, MyPlayerNum );

  if ( DOS_TXT ) {
    ds.Replace( _T("\n"), _T("\r\n"), true );
  }
  wxLogMessage( _T("20") );

  wxString path = wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator();

  wxFile f( wxString(path + _T("script_debug.txt")), wxFile::write );
  f.Write( _T("[Script Start]") + s );
  f.Write( ds );
  f.Close();
  wxLogMessage( _T("21") );

  */



  //return s;
}


wxString Spring::WriteSPScriptTxt( SinglePlayerBattle& battle )
{
  wxString ret;
  TDFWriter tdf(ret);
  std::vector<int> AllyConv;

  int NumAllys = 0;
  int PlayerTeam = -1;

  long startpostype;
  battle.CustomBattleOptions()->getSingleValue( _T("startpostype"), EngineOption ).ToLong( &startpostype );

  wxLogMessage( _T("StartPosType=%d"), (int)startpostype );


  for ( unsigned int i = 0; i < battle.GetNumBots(); i++ ) {
    BattleBot* bot;
    bot = battle.GetBot( i );
    ASSERT_LOGIC( bot != 0, _T("bot == 0") );
    if ( bot->aidll == _T("") ) PlayerTeam = i;
    if(bot->bs.ally>int(AllyConv.size())-1){
      AllyConv.resize(bot->bs.ally+1,-1);
    }
    if( AllyConv[bot->bs.ally] == -1 ) AllyConv[bot->bs.ally] = NumAllys++;
  }

  ASSERT_LOGIC( PlayerTeam != -1, _T("no player found") );

  // Start generating the script.
  //s  = wxString::Format( _T("[GAME]\n{\n") );
  tdf.EnterSection(_T("GAME"));

  tdf.Append(_T("Mapname"),battle.GetHostMapName());

  tdf.Append(_T("GameType"),usync()->GetModArchive(usync()->GetModIndex(battle.GetHostModName())));

  unsigned long uhash;
  battle.LoadMod().hash.ToULong(&uhash);

  tdf.Append(_T("ModHash"),int(uhash));

  wxStringTripleVec optlistEng;
  battle.CustomBattleOptions()->getOptions( &optlistEng, EngineOption );
  for (wxStringTripleVec::iterator it = optlistEng.begin(); it != optlistEng.end(); ++it)
  {
    tdf.Append(it->first, it->second.second);
  }

  tdf.Append(_T("HostIP"),"localhost");

  tdf.Append(_T("HostPort"),"8452");

  tdf.Append(_T("MyPlayerNum"),"0");

  tdf.Append(_T("NumPlayers"),"1");

  ///TODO: investigate if this is correct.
  tdf.Append(_T("NumTeams"),battle.GetNumBots());

  tdf.Append(_T("NumAllyTeams"),NumAllys);

  tdf.EnterSection(_T("PLAYER0"));
    tdf.Append(_T("name"),"Player");
    tdf.Append(_T("Spectator"),"0");
    tdf.Append(_T("team"),PlayerTeam);
  tdf.LeaveSection();

  for ( unsigned int i = 0; i < battle.GetNumBots(); i++ ) {
    BattleBot* bot;
    if ( startpostype == ST_Pick) bot = battle.GetBot( i );
    else bot = battle.GetBotByStartPosition( i );
    ASSERT_LOGIC( bot != 0, _T("bot == 0") );

    tdf.EnterSection(_T("TEAM")+i2s(i));

    if ( startpostype == ST_Pick ){
      tdf.Append(_T("StartPosX"),bot->posx);
      tdf.Append(_T("StartPosZ"),bot->posy);
    }

    tdf.Append(_T("TeamLeader"),"0");
    tdf.Append(_T("AllyTeam"),AllyConv[bot->bs.ally]);

    wxString colourstring =
      TowxString( bot->bs.colour.Red()/255.0f ) + _T(' ') +
      TowxString( bot->bs.colour.Green()/255.0f ) + _T(' ') +
      TowxString( bot->bs.colour.Blue()/255.0f );
    tdf.Append(_T("RGBColor"), colourstring);

    tdf.Append(_T("Side"),usync()->GetSideName(battle.GetHostModName(), bot->bs.side));

    tdf.Append(_T("Handicap"),bot->bs.handicap);

    if ( bot->aidll != _T("") ) {
      wxString ai = bot->aidll;
      /*if ( wxFileName::FileExists( sett().GetSpringDir() + wxFileName::GetPathSeparator() + _T("AI") + wxFileName::GetPathSeparator() + _T("Bot-libs") + wxFileName::GetPathSeparator() + ai + _T(".dll") ) ) {
        ai += _T(".dll");
      } else {
        ai += _T(".so");
      }*/
      tdf.Append(_T("AIDLL"),ai);
      //s += _T("\t\tAIDLL=AI/Bot-libs/") + ai + _T(";\n");
    }
    //s +=  _T("\t}\n");
    tdf.LeaveSection();
  }

  for ( int i = 0; i < NumAllys; i++ ) {
    tdf.EnterSection(_T("ALLYTEAM")+i2s(i));
    tdf.Append(_T("NumAllies"),"0");
    tdf.LeaveSection();
  }
  wxArrayString units = battle.DisabledUnits();
  tdf.Append(_T("NumRestrictions"),units.GetCount());

  tdf.EnterSection(_T("RESTRICT"));

  for ( unsigned int i = 0; i < units.GetCount(); i++) {
    tdf.Append(_T("Unit")+i2s(i),units[i].c_str());
    tdf.Append(_T("Limit")+i2s(i),_T("0"));
  }
  tdf.LeaveSection();

  tdf.EnterSection(_T("mapoptions"));
  wxStringTripleVec optlistMap;
  battle.CustomBattleOptions()->getOptions( &optlistMap, MapOption );
  for (wxStringTripleVec::iterator it = optlistMap.begin(); it != optlistMap.end(); ++it)
  {
    tdf.Append(it->first, it->second.second);
  }
  tdf.LeaveSection();

  tdf.EnterSection(_T("modoptions"));
  wxStringTripleVec optlistMod;
  battle.CustomBattleOptions()->getOptions( &optlistMod, ModOption );
  for (wxStringTripleVec::iterator it = optlistMod.begin(); it != optlistMod.end(); ++it)
  {
    tdf.Append(it->first, it->second.second);
  }
  tdf.LeaveSection();

  tdf.LeaveSection();

  return ret;
/*
  if ( DOS_TXT ) {
    s.Replace( _T("\n"), _T("\r\n"), true );
  }

  return s;
  */
}
