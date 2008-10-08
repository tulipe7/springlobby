#include <wx/intl.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/textdlg.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/filename.h>
#if wxUSE_TOGGLEBTN
    #include <wx/tglbtn.h>
#endif

#include <stdexcept>

#include "replaytab.h"
#include "replaylistctrl.h"
#include "../ui.h"
#include "../chatpanel.h"
#include "../utils.h"
#include "../uiutils.h"
#include "../settings.h"
#include "../iunitsync.h"
#include "../mapctrl.h"
#include "../userlistctrl.h"

#include "replayfilter.h"
#include "../iconimagelist.h"

#include "../settings++/custom_dialogs.h"


#define REPLAYLIST_COLOUMNCOUNT 10

BEGIN_EVENT_TABLE(ReplayTab, wxPanel)

  EVT_BUTTON              ( REPLAY_WATCH             , ReplayTab::OnWatch        )
  EVT_BUTTON              ( REPLAY_RELOAD             , ReplayTab::OnReload        )
  EVT_BUTTON              ( REPLAY_DELETE            , ReplayTab::OnDelete    )
  EVT_LIST_ITEM_SELECTED  ( RLIST_LIST               , ReplayTab::OnSelect      )
  EVT_CHECKBOX            ( REPLAY_LIST_FILTER_ACTIV , ReplayTab::OnFilterActiv )
#if  wxUSE_TOGGLEBTN
  EVT_TOGGLEBUTTON        ( REPLAY_LIST_FILTER_BUTTON, ReplayTab::OnFilter  )
#else
  EVT_CHECKBOX            ( REPLAY_LIST_FILTER_BUTTON , ReplayTab::OnFilter )
#endif


END_EVENT_TABLE()

ReplayTab::ReplayTab( wxWindow* parent, Ui& ui ) :
  wxPanel( parent, -1 ),
  m_ui(ui),
  m_sel_replay_id(0)
{
    //TODO this shouldnÄT be here
    m_ui.ReloadUnitSync();

    m_replays = new ReplayList ( *this );

    wxBoxSizer* m_main_sizer;
    m_main_sizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* m_filter_sizer;
    m_filter_sizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* m_replaylist_sizer;
    m_replaylist_sizer = new wxBoxSizer( wxVERTICAL );

    m_replay_listctrl = new ReplayListCtrl( this, *m_replays );
    m_replaylist_sizer->Add( m_replay_listctrl, 1, wxALL|wxEXPAND, 5 );

    m_main_sizer->Add( m_replaylist_sizer, 1, wxEXPAND, 5 );;

    wxBoxSizer* m_info_sizer;
    m_info_sizer = new wxBoxSizer( wxHORIZONTAL );

    m_minimap = new MapCtrl( this, 100, 0, m_ui, true, true, false, false );
    m_info_sizer->Add( m_minimap, 0, wxALL, 5 );

    wxFlexGridSizer* m_data_sizer;
    m_data_sizer = new wxFlexGridSizer( 4, 2, 0, 0 );

    m_map_lbl = new wxStaticText( this, wxID_ANY, _("Map:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_map_lbl, 0, wxALL, 5 );

    m_map_text = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_map_text, 0, wxALL, 5 );

    m_mod_lbl = new wxStaticText( this, wxID_ANY, _("Mod:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_mod_lbl, 0, wxALL, 5 );

    m_mod_text = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_mod_text, 0, wxALL, 5 );

    m_players_lbl = new wxStaticText( this, wxID_ANY, _("Players:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_players_lbl, 0, wxALL, 5 );

    m_players_text = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_data_sizer->Add( m_players_text, 0, wxALL, 5 );

    m_info_sizer->Add( m_data_sizer, 1, wxEXPAND, 5 );

    m_players = new UserListctrl( this, _T("replayusers"),REPLAY_USER_LIST );
    m_info_sizer->Add( m_players , 1, wxEXPAND, 5 );

    m_main_sizer->Add( m_info_sizer, 0, wxEXPAND, 5 );


    m_filter = new ReplayListFilter( this , wxID_ANY, this ,wxDefaultPosition, wxSize( -1,-1 ), wxEXPAND );
    m_filter_sizer->Add( m_filter, 0, wxEXPAND, 5);

    m_main_sizer->Add( m_filter_sizer, 0, wxEXPAND, 5);

    m_buttons_sep = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    m_main_sizer->Add( m_buttons_sep, 0, wxALL|wxEXPAND, 5 );

    wxBoxSizer* m_buttons_sizer;
    m_buttons_sizer = new wxBoxSizer( wxHORIZONTAL );

    #if  wxUSE_TOGGLEBTN
        m_filter_show = new wxToggleButton( this, REPLAY_LIST_FILTER_BUTTON , wxT(" Filter "), wxDefaultPosition , wxSize( -1,28 ), 0 );
    #else
        m_filter_show = new wxCheckBox( this, REPLAY_LIST_FILTER_BUTTON , wxT(" Filter "), wxDefaultPosition , wxSize( -1,28 ), 0 );
    #endif

    m_buttons_sizer->Add( m_filter_show, 0, 0, 5 );

    m_filter_activ = new wxCheckBox( this, REPLAY_LIST_FILTER_ACTIV , wxT("Activated"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttons_sizer->Add( m_filter_activ, 1, wxALL|wxEXPAND, 5 );

    #ifdef HAVE_WX26
        m_filter_activ->Disable();
    #endif

    m_buttons_sizer->Add( 0, 0, 1, wxEXPAND, 0 );

    m_watch_btn = new wxButton( this, REPLAY_WATCH, _("Watch"), wxDefaultPosition, wxSize( -1,28 ), 0 );
    m_buttons_sizer->Add( m_watch_btn, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

    m_delete_btn = new wxButton( this, REPLAY_DELETE, _("Delete"), wxDefaultPosition, wxSize( -1,28 ), 0 );
    m_buttons_sizer->Add( m_delete_btn, 0, wxBOTTOM|wxRIGHT, 5 );

    m_reload_btn = new wxButton( this, REPLAY_RELOAD, _("Reload list"), wxDefaultPosition, wxSize( -1,28 ), 0 );
    m_buttons_sizer->Add( m_reload_btn, 0, wxBOTTOM|wxRIGHT, 5 );

    m_main_sizer->Add( m_buttons_sizer, 0, wxEXPAND, 5 );

    m_filter->Hide();

    this->SetSizer( m_main_sizer );
    this->Layout();

    //replay adding is now controlled by replaylist
    //AddAllReplays();
    m_replays->LoadReplays();

    //none selected --> shouldn't watch that
    m_watch_btn->Enable( false );

}


ReplayTab::~ReplayTab()
{
    if (m_filter != 0)
        m_filter->SaveFilterValues();
}

void ReplayTab::AddAllReplays()
{
    for (unsigned int i = 0; i < m_replays->GetNumReplays(); ++i)
    {
        Replay r = m_replays->GetReplay(i);
        AddReplay( r );
    }
}

void ReplayTab::AddReplay( Replay& replay ) {

    if ( m_filter->GetActiv() && !m_filter->FilterReplay( replay ) ) {
        return;
    }

    int index = m_replay_listctrl->InsertItem( 0, replay.date );
    ASSERT_LOGIC( index != -1, _T("index = -1") );

    m_replay_listctrl->SetItemData(index, (long)replay.id );

    if (index != -1)
        UpdateReplay( replay, index );
    else
        wxLogMessage( _T("Replay index == -1 at update") );

}


void ReplayTab::RemoveReplay( Replay& replay ) {
    try{
    for (int i = 0; i < m_replay_listctrl->GetItemCount() ; i++ ) {
        if ( replay.id == (int)m_replay_listctrl->GetItemData( i ) ) {
            m_replay_listctrl->DeleteItem( i );
            if ( m_replay_listctrl->GetSelectedIndex() == i ){
                m_replay_listctrl->SetSelectedIndex(-1);
                m_sel_replay_id = -1;
            }
            break;
        }
    }
    } catch (...) {
        wxLogMessage( _T("exception on remove replay") );
    }
}

void ReplayTab::UpdateReplay( Replay& replay )
{

    if ( m_filter->GetActiv() && !m_filter->FilterReplay( replay ) ) {
        RemoveReplay( replay );
        return;
    }

    int index = -1;
    for (int i = 0; i < m_replay_listctrl->GetItemCount() ; i++ ) {
        if ( replay.id == (int)m_replay_listctrl->GetItemData( i ) ) {
          index = i;
          break;
        }
    }

    if (index != -1)
        UpdateReplay( replay, index );
    else
        AddReplay( replay );

}

void ReplayTab::UpdateReplay( Replay& replay, const int index )
{

    ASSERT_LOGIC( index != -1, _T("index = -1") );

    wxString duration = wxString::Format(_T("%02ld:%02ld:%02ld"), replay.duration / 3600,
                        (replay.duration%3600)/60, (replay.duration%60)/60 ) ;
    m_replay_listctrl->SetItem( index, 0, replay.date );
    m_replay_listctrl->SetItem( index, 1, replay.battle.GetHostModName() );
    m_replay_listctrl->SetItem( index, 2, replay.battle.GetHostMapName() );
    m_replay_listctrl->SetItem( index, 3, wxString::Format(_T("%d"),replay.battle.GetNumUsers() - replay.battle.GetSpectators() ) );
    m_replay_listctrl->SetItem( index, 4, duration );
    m_replay_listctrl->SetItem( index, 5, replay.SpringVersion );
    m_replay_listctrl->SetItem( index, 6, wxString::Format( _T("%d KB"),replay.size/1024 ) );
    m_replay_listctrl->SetItem( index, 7, replay.Filename.AfterLast( wxFileName::GetPathSeparator() ) );

    //  if ( &replay == m_sel_replay ) SelectReplay( m_sel_replay );
    //  m_replay_listctrl->Sort();

}


void ReplayTab::RemoveAllReplays() {
  m_sel_replay_id = 0;

  m_replay_listctrl->DeleteAllItems();
}


void ReplayTab::UpdateList() {

  for (unsigned int i = 0; i < m_replays->GetNumReplays(); ++i) {
    Replay b = m_replays->GetReplay(i);

    UpdateReplay(b);
  }
}


void ReplayTab::SetFilterActiv( bool activ )
{
  m_filter->SetActiv( activ );
  m_filter_activ->SetValue( activ );
}

void ReplayTab::OnFilter( wxCommandEvent& event )
{
  if (m_filter_show->GetValue()) {
    m_filter->Show(  );
    this->Layout();
  }
  else {
    m_filter->Hide(  );
    this->Layout();
  }
}

void ReplayTab::OnWatch( wxCommandEvent& event )
{
    Replay rep = m_replays->GetReplayById(m_sel_replay_id);
    bool watchable = rep.battle.MapExists() && rep.battle.ModExists();
    if ( watchable )
        m_ui.WatchReplay( rep.Filename );
    else {
        #ifdef NO_TORRENT_SYSTEM
            wxString downloadProc = _("Do you want me to take you to the download page?");
        #else
            wxString downloadProc = _("Should i try to downlaod it for you?\nYou can see the progress in the \"Download Manager\" tab.");
        #endif

        OfflineBattle& battle = rep.battle;

        if ( !battle.ModExists() ) {
            if (customMessageBox( SL_MAIN_ICON, _("You need to download the mod before you can watch this replay.\n\n") + downloadProc, _("Mod not available"), wxYES_NO | wxICON_QUESTION ) == wxYES ) {
                wxString modhash = battle.GetHostModHash();
                wxString modname = battle.GetHostModName();
                m_ui.DownloadMod ( modhash, modname );
            }
            return;
        }

        if ( !battle.MapExists() ) {
            if (customMessageBox(SL_MAIN_ICON, _("You need to download the map to be able to watch this replay.\n\n") + downloadProc, _("Map not available"), wxYES_NO | wxICON_QUESTION ) == wxYES ) {
                wxString maphash = battle.GetHostMapHash();
                wxString mapname = battle.GetHostMapName();
                m_ui.DownloadMap ( maphash, mapname );
            }
        }
    }

}

void ReplayTab::OnDelete( wxCommandEvent& event )
{
    Replay rep = m_replays->GetReplayById(m_sel_replay_id);
    wxString fn = rep.Filename;
    if ( !m_replays->DeleteReplay( m_sel_replay_id ) )
        customMessageBoxNoModal(SL_MAIN_ICON, _("Could not delete Replay: ") + fn,
            _("Error") );
    else
        RemoveReplay( rep );
}

void ReplayTab::OnFilterActiv( wxCommandEvent& event )
{
    m_filter->SetActiv( m_filter_activ->GetValue() );
}

void ReplayTab::OnSelect( wxListEvent& event )
{
    wxLogDebugFunc( _T("") );
    if ( event.GetIndex() == -1 ) {
        m_sel_replay_id = 0;
        m_watch_btn->Enable( false );
    } else {
        m_watch_btn->Enable( true );
        int index = event.GetIndex();
        long data = m_replay_listctrl->GetItemData( index );
        Replay& rep = m_replays->GetReplay( data );
        m_sel_replay_id = rep.id;
        m_players_text->SetLabel(_T(""));
        m_map_text->SetLabel(rep.battle.GetHostMapName());
        m_mod_text->SetLabel(rep.battle.GetHostModName());
        m_minimap->SetBattle( &(rep.battle) );
        m_minimap->UpdateMinimap();
        m_players->Clear();
        for ( OfflineBattle::UserVecCIter it = rep.battle.GetFirstUser();
            it != rep.battle.GetLastUser(); ++it )
        {
            UserListctrl::UserData ud ( it->GetNick() , it->GetCountry() );
            m_players->AddUser( ud );
        }
    }
}

void ReplayTab::ReloadList()
{
    /// should be changed to use delayed load once perf testing is done
    wxDateTime dt = wxDateTime::UNow();
    m_replays->RemoveAll();
    m_replays->LoadReplays();
    long sec = (wxDateTime::UNow() - dt).GetMilliseconds().ToLong();
    if ( sec > 0 )
        customMessageBoxNoModal(SL_MAIN_ICON, wxString::Format( _T("List reloaded in %d milli seconds"),sec ) );
}

void ReplayTab::OnReload( wxCommandEvent& event )
{
    ReloadList();
}