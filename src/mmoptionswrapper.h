#ifndef MMOPTIONSORAPPER_H_
#define MMOPTIONSORAPPER_H_

#include "iunitsync.h"

#include <vector>
#include <utility>
#include <wx/string.h>

struct GameOptions;

class wxFileConfig;

class mmSectionTree {

    public:
        mmSectionTree();
        ~mmSectionTree();

        void AddSection( const mmOptionSection );
        mmOptionSection GetSection( const wxString& key );

        typedef std::vector< mmOptionSection > SectionVector;

//        SectionVector GetSectionVector();

        void Clear();

    protected:
        //map key -> option
        typedef std::map< wxString, mmOptionSection > SectionMap;
        SectionMap m_section_map;
        typedef wxFileConfig ConfigType;
        ConfigType* m_tree;

        void AddSection ( const wxString& path, const mmOptionSection& section );
        wxString FindParentpath ( const wxString& parent_key );
        bool FindRecursive( const wxString& parent_key, wxString& path );

};

class OptionsWrapper
{
public:
    //! public types
    typedef std::pair < wxString,wxString> wxStringPair;
    typedef std::pair < wxString, wxStringPair> wxStringTriple;
    typedef std::vector<wxStringPair> wxStringPairVec;
    typedef std::vector<wxStringTriple> wxStringTripleVec;
    typedef std::map<wxString,wxString> wxStringMap;
    typedef std::map<int, GameOptions> GameOptionsMap;
    typedef std::map<int, mmSectionTree> mmSectionTreeMap;

    //! enum to differentiate option category easily at runtime
    enum GameOption{
      PrivateOptions  = 3,
      EngineOption = 2,
        MapOption    = 1,
        ModOption    = 0,
        LastOption = 4
    };// should reflect: optionCategoriesCount

	//does nothing
	OptionsWrapper();
	virtual ~OptionsWrapper();
	//! just calls loadOptions(MapOption,mapname)
	bool loadMapOptions(wxString mapname);

	//! load corresponding options through unitsync calls
	/*!
	 * the containers for corresponding flag are recreated and then gets the number of options from unitsync
	 * and adds them one by one  to the appropiate container
	 * \param flag decides which type of option to load
	 * \return true if load successful, false otherwise
	 */
	bool loadOptions(GameOption flag, const wxString& name );
	//! checks if given key can be found in specified container
	/*!
	 * \param key the key that should be checked for existance in containers
	 * \param flag which GameOption conatiner should be searched
	 * \param showError if true displays a messagebox if duplicate key is found
	 * \param optType will contain the corresponding OptionType if key is found, opt_undefined otherwise
	 * \return true if key is found, false otherwise
	 */
	bool keyExists(wxString key,GameOption flag,bool showError, OptionType& optType);
	//! given a vector of key/value pairs sets the appropiate options to new values
	/*!	Every new value is tested for meeting boundary conditions, type, etc.
	 * If test fails error is logged and false is returned.
	 * \param values the wxStringPairVec containing key / new value pairs
	 * \param flag which OptionType is to be processed
	 * \return false if ANY error occured, true otherwise
	 */
	bool setOptions(wxStringPairVec* values,GameOption flag);
	//! get all options of one GameOption
	/*! the output has the following format: < wxString , Pair < wxString , wxString > >
	 * meaning < key , < name , value > >
	 * \param triples this will contain the options after the function
	 * \param flag which OptionType is to be processed
	 */
	wxStringTripleVec getOptions( GameOption flag );
	//! similar to getOptions, instead of vector a map is used and the name is not stored
	std::map<wxString,wxString> getOptionsMap(GameOption);
	//! recreates ALL containers
	void unLoadOptions();
	//! recreates the containers of corresponding flag
	void unLoadOptions(GameOption flag);

	//! returns value of specified key
	/*! searches all containers for key
	 * \return value of key if key found, "" otherwise
	 */
	wxString getSingleValue(wxString key);
	//! returns value of specified key
	/*! searches containers of type flag for key
	 * \return value of key if key found, "" otherwise
	 */

	wxString getSingleValue(wxString key, GameOption flag);

	wxString getDefaultValue(wxString key, GameOption flag);

	//! sets a single option in specified container
	/*! \return true if success, false otherwise */
	bool setSingleOption(wxString key, wxString value, GameOption modmapFlag);
	//! same as before, but tries all containers
	bool setSingleOption(wxString key, wxString value);

	//! returns the option type of specified key (all containers are tried)
	OptionType GetSingleOptionType (wxString key);

	//!returns the cbx_choice associated w current listoption
	wxString GetNameListOptValue(wxString key, GameOption flag);

	//! returns the listitem key associated with listitem name
	wxString GetNameListOptItemKey(wxString optkey, wxString itemname, GameOption flag);

	GameOptionsMap opts;

	//! after loading sections into map, parse them into tree
	void ParseSectionMap( mmSectionTree& section_tree, const IUnitSync::OptionMapSection& section_map );
protected:
	//! used for code clarity in setOptions()
	bool setSingleOptionTypeSwitch(wxString key, wxString value, GameOption modmapFlag, OptionType optType);

	mmSectionTreeMap m_sections;

};

#endif /*MMOPTIONSORAPPER_H_*/

/**
    This file is part of SpringLobby,
    Copyright (C) 2007-09

    springsettings is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.

    springsettings is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SpringLobby.  If not, see <http://www.gnu.org/licenses/>.
**/

