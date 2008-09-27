
#ifndef SPRINGLOBBY_HEADERGUARD_GROUPOPTIONSPANEL_H
#define SPRINGLOBBY_HEADERGUARD_GROUPOPTIONSPANEL_H

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/statbox.h>
#include <wx/panel.h>

class ColorButton;

class GroupOptionsPanel : public wxPanel
{
	DECLARE_EVENT_TABLE()

	protected:
		enum
		{
			REMOVE_GROUP = 1000,
			RENAME_GROUP,
			ADD_GROUP,
			GROUPS_LIST,
			NOTIFY_LOGIN,
			IGNORE_CHAT,
			NOTIFY_HOST,
			IGNORE_PM,
			NOTIFY_STATUS,
			AUTOCKICK,
			NOTIFY_HIGHLIGHT,
			HIGHLIGHT_COLOR,
			USERS_LIST,
			ADD_USER,
			REMOVE_USER,
		};

		wxListBox* m_group_list;
		wxButton* m_remove_group_button;
		wxButton* m_rename_group_button;

		wxButton* m_add_group_button;
		wxPanel* m_group_panel;
		wxCheckBox* m_login_notify_check;
		wxCheckBox* m_ignore_chat_check;
		wxCheckBox* m_notify_host_check;
		wxCheckBox* m_ignore_pm_check;
		wxCheckBox* m_notify_status_check;
		wxCheckBox* m_autokick_check;
		wxCheckBox* m_highlight_check;
		wxStaticText* m_highlight_colorstaticText;
		ColorButton* m_highlight_color_button;
		wxListBox* m_user_list;
		wxButton* m_add_user_button;
		wxButton* m_remove_user_button;

    wxString m_current_group;

		void OnRemoveGroup( wxCommandEvent& event );
		void OnRenameGroup( wxCommandEvent& event );
		void OnAddNewGroup( wxCommandEvent& event );
		void OnGroupListSelectionChange( wxCommandEvent& event );
		void OnGroupActionsChange( wxCommandEvent& event );
		void OnHighlightColorClick( wxCommandEvent& event );
		void OnUsersListSelectionChange( wxCommandEvent& event );
		void OnAddUsers( wxCommandEvent& event );
		void OnRemoveUser( wxCommandEvent& event );

		void Initialize();

		void ShowGroup( const wxString& group );
		void ReloadUsersList();
		void ReloadGroupsList();
		wxString GetFirstGroupName();

	public:
		GroupOptionsPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 656,537 ), long style = wxTAB_TRAVERSAL );
		~GroupOptionsPanel();

};

#endif
