
#include "selectusersdialog.h"

#include "iconimagelist.h"
#include "ui.h"
#include "userlist.h"
#include "server.h"
#include "user.h"
#include "utils.h"

#include <wx/tokenzr.h>


BEGIN_EVENT_TABLE( SelectUsersDialog, wxDialog )
  EVT_TEXT( FILTER_TEXT, SelectUsersDialog::OnNameFilterChange )
  EVT_LIST_ITEM_ACTIVATED( NICK_LIST, SelectUsersDialog::OnNameActivated )
  EVT_LIST_ITEM_DESELECTED( NICK_LIST, SelectUsersDialog::OnNameDeselected )
  EVT_LIST_ITEM_SELECTED( NICK_LIST, SelectUsersDialog::OnNameSelected )
  EVT_BUTTON( wxID_CANCEL, SelectUsersDialog::OnCancel )
  EVT_BUTTON( wxID_OK, SelectUsersDialog::OnOk )
END_EVENT_TABLE()

SelectUsersDialog::SelectUsersDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
  this->SetSizeHints( wxDefaultSize, wxDefaultSize );

  wxBoxSizer* mainSizer;
  mainSizer = new wxBoxSizer( wxVERTICAL );

  m_users_panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
  wxBoxSizer* bUsersListSizer;
  bUsersListSizer = new wxBoxSizer( wxVERTICAL );

  wxBoxSizer* filterSizer;
  filterSizer = new wxBoxSizer( wxHORIZONTAL );

  m_filter_names_staticText = new wxStaticText( m_users_panel, wxID_ANY, _("Filter names"), wxDefaultPosition, wxDefaultSize, 0 );
  m_filter_names_staticText->Wrap( -1 );
  filterSizer->Add( m_filter_names_staticText, 0, wxALL|wxALIGN_BOTTOM, 5 );

  m_name_filter_text = new wxTextCtrl( m_users_panel, FILTER_TEXT, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  m_name_filter_text->SetToolTip( _("Enter text filter to filter the online users list") );

  filterSizer->Add( m_name_filter_text, 1, wxLEFT|wxRIGHT|wxTOP, 5 );

  bUsersListSizer->Add( filterSizer, 0, wxEXPAND, 5 );

  m_user_list = new wxListCtrl( m_users_panel, NICK_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER|wxLC_SORT_ASCENDING );
  bUsersListSizer->Add( m_user_list, 1, wxALL|wxEXPAND, 5 );

  m_users_panel->SetSizer( bUsersListSizer );
  m_users_panel->Layout();
  bUsersListSizer->Fit( m_users_panel );
  mainSizer->Add( m_users_panel, 1, wxEXPAND, 5 );

  wxBoxSizer* bNameSizer;
  bNameSizer = new wxBoxSizer( wxHORIZONTAL );

  m_name_staticText = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
  m_name_staticText->Wrap( -1 );
  bNameSizer->Add( m_name_staticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

  m_selection_text = new wxTextCtrl( this, NAME_TEXT, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  bNameSizer->Add( m_selection_text, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

  mainSizer->Add( bNameSizer, 0, wxEXPAND, 5 );

  m_buttons_hr = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
  mainSizer->Add( m_buttons_hr, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

  m_dialog_buttons = new wxStdDialogButtonSizer();
  m_dialog_buttonsOK = new wxButton( this, wxID_OK );
  m_dialog_buttons->AddButton( m_dialog_buttonsOK );
  m_dialog_buttonsCancel = new wxButton( this, wxID_CANCEL );
  m_dialog_buttons->AddButton( m_dialog_buttonsCancel );
  m_dialog_buttons->Realize();
  mainSizer->Add( m_dialog_buttons, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

  this->SetSizer( mainSizer );
  this->Layout();

  Initialize();
}

SelectUsersDialog::~SelectUsersDialog()
{
  ClearList();
}

void SelectUsersDialog::Initialize()
{
  m_user_list->InsertColumn(0, _("Nickname"), wxLIST_FORMAT_LEFT, 300);
  m_user_list->SetImageList( &icons(), wxIMAGE_LIST_SMALL );
  m_user_list->SetImageList( &icons(), wxIMAGE_LIST_NORMAL );
  m_user_list->SetImageList( &icons(), wxIMAGE_LIST_STATE );
  PopulateUsersList();
}

void SelectUsersDialog::PopulateUsersList()
{
  ClearList();
  if ( ui().IsConnected() ) {
    const UserList& userlist = ui().GetServer().GetUserList();

    m_user_list->Freeze();
    for ( unsigned int i = 0; i < userlist.GetNumUsers(); ++i) {
      wxString name = userlist.GetUser( i ).GetNick();
      if ( !useractions().IsKnown( name ) && !ui().IsThisMe( name ) ) {
        wxString country = userlist.GetUser( i ).GetCountry();
        AddUserToList( name, country );
      }
    }
    Sort();
    m_user_list->Thaw();

  }
}

void SelectUsersDialog::ClearList()
{
  m_user_list->Freeze();
  long item = -1;
  while ( true ) {
    item = m_user_list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE);
    if ( item == -1 )
      break;
    RemoveUserFromList( item );
    item = -1;
  }
  m_user_list->Thaw();
}

long SelectUsersDialog::AddUserToList( const wxString& nick, const wxString& flag )
{
  return AddUserToList(nick, icons().GetFlagIcon(flag) );
}

long SelectUsersDialog::AddUserToList( const wxString& nick, const int& flag )
{
  long item = m_user_list->InsertItem(0, nick, flag );
  m_user_list->SetItemData(item, (wxUIntPtr)(new wxString(nick)));
  return item;
}

void SelectUsersDialog::RemoveUserFromList( long item )
{
  delete (wxString*)m_user_list->GetItemData(item);
  m_user_list->DeleteItem(item);
}

void SelectUsersDialog::UpdateUsersList()
{
}

void SelectUsersDialog::UpdateSelection()
{
  wxSortedArrayString text = GetSelectionFromText();
  for ( unsigned int i = 0; i < m_selection.Count(); i++ ) {
    if ( text.Index(m_selection[i], false) == wxNOT_FOUND ) text.Insert( m_selection[i], 0 );
  }

  m_selection_text->SetValue(BuildSelectionText( text ));
}

wxString SelectUsersDialog::BuildSelectionText( const wxSortedArrayString& sel )
{
  wxString str;
  for ( unsigned int i = 0; i < sel.Count(); i++ ) {
    if ( str != wxEmptyString ) str += _T(";");
    str += sel[i];
  }
  return str;
}

wxSortedArrayString SelectUsersDialog::GetSelectionFromText()
{
  wxSortedArrayString s;
  wxStringTokenizer st(m_selection_text->GetValue(), _T(";, "), wxTOKEN_DEFAULT);
  while ( st.HasMoreTokens() ) {
    s.Add(st.GetNextToken());
  }
  return s;
}

wxSortedArrayString SelectUsersDialog::GetUsers(wxWindow* parent)
{
  SelectUsersDialog dlg(parent);
  if ( dlg.ShowModal() == wxID_OK ) {
    return dlg.GetSelection();
  } else {
    wxSortedArrayString s;
    return s;
  }
}

int SelectUsersDialog::ShowModal()
{
  m_selection.Clear();
  return wxDialog::ShowModal();
}

void SelectUsersDialog::OnNameFilterChange( wxCommandEvent& event )
{
  long item = -1;
  wxString filter = m_name_filter_text->GetValue();

  wxArrayString del;

  m_user_list->Freeze();
  while ( true ) {
    item = m_user_list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE);
    if ( item == -1 )
      break;

    wxString name = m_user_list->GetItemText(item);
    if ( name == wxEmptyString ) continue;

    filter.MakeLower();
    name.MakeLower();

    if ( (filter != wxEmptyString) && (!name.Contains(filter)) ) {
      wxListItem listItem;
      listItem.SetId(item);
      m_user_list->GetItem(listItem);
      int flag = listItem.GetImage();
      del.Add(m_user_list->GetItemText(item));
      m_filtered_users.Add( name +_T(" ") + i2s(flag) );
    }

  }

  if ( del.Count() == (size_t)m_user_list->GetItemCount() ) {
    m_user_list->DeleteAllItems();
  } else {
    for ( unsigned int i = 0; i < del.Count(); i++ ) {
      long item = m_user_list->FindItem(-1, del[i]);
      RemoveUserFromList(item);
    }
  }

  wxSortedArrayString sel = GetSelection();

  for ( int i = m_filtered_users.Count() - 1; i >= 0; i-- )
  {
    wxString line = m_filtered_users[i];
    int sep = line.Find(' ');
    wxString name = line.Mid(0, sep);
    if ( name.Contains(filter) || (filter == wxEmptyString) ) {
      int flag = (int)s2l( line.Mid(sep+1) ); // Flag is never < 0 or > intmax
      long item = AddUserToList( name, flag );
      m_filtered_users.RemoveAt(i);
      if ( sel.Index(name, false) != wxNOT_FOUND )
        m_user_list->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
  }
  Sort();
  m_user_list->Thaw();
}

void SelectUsersDialog::OnNameActivated( wxListEvent& event )
{
  m_selection.Clear();
  m_selection.Add(event.GetText());
  EndModal(wxID_OK);
}

void SelectUsersDialog::OnNameDeselected( wxListEvent& event )
{
  m_selection.Remove(event.GetText());

  wxSortedArrayString text = GetSelectionFromText();
  text.Remove(event.GetText());

  m_selection_text->SetValue(BuildSelectionText( text ));
  UpdateSelection();
}

void SelectUsersDialog::OnNameSelected( wxListEvent& event )
{
  m_selection.Remove(event.GetText());
  m_selection.Add(event.GetText());
  UpdateSelection();
}

void SelectUsersDialog::OnCancel( wxCommandEvent& event )
{
  EndModal(wxID_CANCEL);
}

void SelectUsersDialog::OnOk( wxCommandEvent& event )
{
  EndModal(wxID_OK);
}


wxSortedArrayString SelectUsersDialog::GetSelection()
{
  return GetSelectionFromText();
}

int wxCALLBACK SelectUsersDialog::CompareName(long item1, long item2, long sortData )
{
  //wxListCtrl* user_list = (wxListCtrl*)sortData;
  wxString* s1 = (wxString*)item1;
  wxString* s2 = (wxString*)item2;
  return (*s1).CmpNoCase(*s2);
}

void SelectUsersDialog::Sort()
{
  m_user_list->Freeze();
  m_user_list->SortItems(CompareName, (wxUIntPtr)m_user_list);
  m_user_list->Thaw();
}
