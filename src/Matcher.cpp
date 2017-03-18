#include <common.h>

///////////////////////////////////////////////////////////////////////////////

#pragma region

typedef uint32_t CharEx;

namespace std {
	template<>
	class regex_traits<CharEx> : public _Regex_traits_base {
	public:
		typedef CharEx _Uelem;
		typedef regex_traits<CharEx> _Myt;
		typedef CharEx char_type;
		typedef basic_string<CharEx> string_type;
		typedef locale locale_type;
		typedef typename string_type::size_type size_type;

		static size_type length( const CharEx* str )
		{
			return string_type( str ).length();
		}
		regex_traits()
		{
		}
		regex_traits( const regex_traits& _Right )
		{
		}
		regex_traits& operator=( const regex_traits& _Right )
		{
			return ( *this );
		}
		CharEx translate( CharEx c ) const
		{
			return c;
		}
		CharEx translate_nocase( CharEx c ) const
		{
			return c;
		}
		template<class Iter>
		string_type transform( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		string_type transform_primary( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		string_type lookup_collatename( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		char_class_type lookup_classname( Iter /*first*/, Iter /*last*/,
			bool /*case = false*/ ) const
		{
			return 0;
		}
		bool isctype( CharEx /*c*/, char_class_type /*type*/ ) const
		{
			return false;
		}
		int value( CharEx /*c*/, int /*base*/ ) const
		{
			throw logic_error( "regex_traits<CharEx>::value" );
			return -1;
		}
		locale_type imbue( locale_type /*newLocale*/ )
		{
			return locale();
		}
		locale_type getloc() const
		{
			return locale();
		}
	};
}

class StringEx : public basic_string<CharEx> {
public:
	StringEx()
	{
	}
	StringEx( const string& str )
	{
		AppendString( str );
	}
	StringEx( const basic_string<CharEx>& str ) :
		basic_string<CharEx>( str )
	{
	}
	StringEx& operator=( const string& str )
	{
		clear();
		AppendString( str );
		return *this;
	}
	StringEx& operator=( const basic_string<CharEx>& str )
	{
		basic_string<CharEx>::operator=( str );
		return *this;
	}

	void AppendString( const string& str )
	{
		reserve( length() + str.length() );
		for( const char c : str ) {
			push_back( static_cast<unsigned char>( c ) );
		}
	}

	string ToString() const
	{
		string result;
		result.reserve( length() );
		for( const CharEx c : *this ) {
			if( c < 128 ) {
				result.push_back( c );
			} else {
				throw logic_error( "StringEx::ToString()" );
			}
		}
		return result;
	}
};

template<>
struct hash<StringEx> {
	typedef StringEx argument_type;
	typedef size_t result_type;
	result_type operator()( argument_type const& str ) const
	{
		return hash<basic_string<CharEx>>{}( str );
	}
};

typedef basic_regex<CharEx> RegexEx;
typedef match_results<StringEx::const_iterator> MatchResultsEx;
#pragma endregion

///////////////////////////////////////////////////////////////////////////////

//namespace Lspl {
//namespace Matching {

///////////////////////////////////////////////////////////////////////////////

typedef size_t TPatternElementIndex;

struct CWordValue {
	static const CharEx ConformAny = static_cast<CharEx>( 128 );
	static const StringEx::size_type ConformStartIndex = 1;

	StringEx Value;

	bool Match( const RegexEx& valueRegex ) const;

	enum TConformity {
		C_None,
		C_Conform,
		C_StrongConform
	};
	TConformity Conform( const CWordValue& other ) const;
};

bool CWordValue::Match( const RegexEx& valueRegex ) const
{
	MatchResultsEx match;
	return regex_match( Value, match, valueRegex );
}

CWordValue::TConformity CWordValue::Conform( const CWordValue& other ) const
{
	debug_check_logic( Value.length() == other.Value.length() );

	TConformity conformity = C_StrongConform;

	for( auto i = ConformStartIndex; i < Value.length(); i++ ) {
		const CharEx c1 = Value[i];
		const CharEx c2 = other.Value[i];
		if( c1 != c2 ) {
			if( c1 == ConformAny || c2 == ConformAny ) {
				conformity = C_Conform;
			} else {
				return C_None;
			}
		}
	}

	return conformity;
}

typedef vector<CWordValue> CWordValues;
typedef CWordValues::size_type TWordValueIndex;

class CWordValueIndices;

struct CWord {
	string text;
	StringEx word;
	CWordValues values;

	const CWordValues& Values() const
	{
		return values;
	}

	bool MatchWord( const RegexEx& wordRegex ) const;
	bool MatchValues( const RegexEx& valueRegex,
		CWordValueIndices& indices ) const;
};

bool CWord::MatchWord( const RegexEx& wordRegex ) const
{
	MatchResultsEx match;
	return regex_match( word, match, wordRegex );
}

typedef vector<CWord> CWords;
typedef CWords::size_type TWordIndex;

class CWordValueIndices {
public:
	explicit CWordValueIndices( const TWordValueIndex count = 0 )
	{
		Reset( count );
	}

	void Reset( const TWordValueIndex count = 0 );
	bool Filled() const
	{
		return !indices.empty();
	}
	void Add( const TWordValueIndex index );
	CWordValueIndices Intersect( const CWordValueIndices& other );

private:
	vector<TWordValueIndex> indices;
};

bool CWord::MatchValues( const RegexEx& valueRegex,
	CWordValueIndices& indices ) const
{
	indices.Reset();
	for( TWordValueIndex index = 0; index < values.size(); index++ ) {
		if( values[index].Match( valueRegex ) ) {
			indices.Add( index );
		}
	}
	return indices.Filled();
}

void CWordValueIndices::Reset( const TWordValueIndex count )
{
	indices.clear();
	indices.reserve( count );
	for( TWordValueIndex i = 0; i < count; i++ ) {
		indices.push_back( i );
	}
}

void CWordValueIndices::Add( const TWordValueIndex index )
{
	debug_check_logic( indices.empty() || indices.back() < index );
	indices.push_back( index );
}

CWordValueIndices CWordValueIndices::Intersect(
	const CWordValueIndices& other )
{
	CWordValueIndices merged;
	merge( indices.cbegin(), indices.cend(),
		other.indices.cbegin(), other.indices.cend(),
		back_inserter( merged.indices ) );
	indices.swap( merged.indices );
	return merged;
}

typedef pair<CWordValueIndices, CWordValueIndices> CArgreement;
typedef pair<TWordIndex, TWordIndex> CWordIndexPair;

template<>
struct hash<CWordIndexPair> {
	typedef CWordIndexPair argument_type;
	typedef size_t result_type;
	result_type operator()( argument_type const& pair ) const
	{
		return ( ( hash<TWordIndex>{}( pair.first ) << 16 )
			^ hash<TWordIndex>{}( pair.second ) );
	}
};

class CWordArgreementsCache {
public:
	CWordArgreementsCache( const CWords& text ) :
		words( text )
	{
	}

	const CArgreement& Agreement( const CWordIndexPair key,
		const bool strong ) const;

private:
	const CWords& words;
	typedef pair<CArgreement, CArgreement> CAgreementPair;
	mutable unordered_map<CWordIndexPair, CAgreementPair> argreements;
};

const CArgreement& CWordArgreementsCache::Agreement(
	const CWordIndexPair key, const bool strong ) const
{
	debug_check_logic( key.first < key.second );
	auto pair = argreements.insert( make_pair( key, CAgreementPair() ) );
	CAgreementPair& result = pair.first->second;
	if( pair.second ) {
		CArgreement argreement;
		CArgreement strongArgreement;

		const CWordValues& values1 = words[key.first].Values();
		const CWordValues& values2 = words[key.first].Values();
		for( size_t index1 = 0; index1 < values1.size(); index1++ ) {
			for( size_t index2 = 0; index2 < values2.size(); index2++ ) {
				switch( values1[index1].Conform( values2[index2] ) ) {
					case CWordValue::C_None:
						break;
					case CWordValue::C_StrongConform:
						strongArgreement.first.Add( index1 );
						strongArgreement.second.Add( index2 );
					case CWordValue::C_Conform:
						argreement.first.Add( index1 );
						argreement.second.Add( index2 );
						break;
				}
			}
		}

		result.first = argreement;
		result.first = strongArgreement;
	}
	return ( strong ? result.second : result.first );
}

class CText : public CWords {
public:
	CText() :
		wordArgreementsCache( static_cast<CWords&>( *this ) )
	{
	}

	TWordIndex NextWord( const TWordIndex wordIndex ) const
	{
		return ( wordIndex + 1 );
	}
	bool HasWord( const TWordIndex wordIndex ) const
	{
		return ( wordIndex < size() );
	}

	const CWordArgreementsCache& Argreements() const
	{
		return wordArgreementsCache;
	}

private:
	CWordArgreementsCache wordArgreementsCache;
};

struct CState;
typedef vector<CState> CStates;
typedef CStates::size_type TStateIndex;

struct CTransition {
	TStateIndex NextState;
	shared_ptr<RegexEx> wordRegex;
	shared_ptr<RegexEx> valueRegex;
	TPatternElementIndex PatternElementIndex;

	bool Match( const CWord& word,
		/* out */ CWordValueIndices& wordValueIndices ) const;
};

bool CTransition::Match( const CWord& word,
	CWordValueIndices& wordValueIndices ) const
{
	wordValueIndices.Reset( word.values.size() );
	if( wordRegex && !word.MatchWord( *wordRegex ) ) {
		return false;
	}
	if( valueRegex ) {
		return word.MatchValues( *valueRegex, wordValueIndices );
	}
	return true;
}

typedef vector<CTransition> CTransitions;

struct CMatching {
	TWordIndex WordIndex;
	TPatternElementIndex PatternElementIndex;
	CWordValueIndices WordValueIndices;
};

typedef vector<CMatching> CMatchings;

struct IUndoAction {
	virtual void Undo() const = 0;
};

class CUndoActions {
public:
	CUndoActions()
	{
	}
	~CUndoActions()
	{
		for( auto ri = undoActions.crbegin();
			ri != undoActions.crend(); ++ri )
		{
			( *ri )->Undo();
		}
	}
	void Add( IUndoAction* undoAction )
	{
		undoActions.emplace_back( undoAction );
	}

private:
	vector<unique_ptr<IUndoAction>> undoActions;
};

struct IAction {
	virtual bool Do( const CText& text, CMatchings& matchings,
		CUndoActions& undoActions ) const = 0;
};

class CActions {
public:
	void Add( shared_ptr<IAction> action );
	bool Do( const CText& text, CMatchings& matchings,
		CUndoActions& undoActions ) const;

private:
	vector<shared_ptr<IAction>> actions;
};

void CActions::Add( shared_ptr<IAction> action )
{
	actions.push_back( action );
}

bool CActions::Do( const CText& text, CMatchings& matchings,
	CUndoActions& undoActions ) const
{
	for( const shared_ptr<IAction>& action : actions ) {
		if( !action->Do( text, matchings, undoActions ) ) {
			return false;
		}
	}
	return true;
}

struct CState {
	CActions Actions;
	CTransitions Transitions;
	vector<TStateIndex> EpsTransitions;
};

class CContext {
public:
	const CText& text;
	const CStates& states;
	CMatchings matchings;

	CContext( const CText& text, const CStates& states ) :
		text( text ), states( states )
	{
		matchings.reserve( 32 );
	}

	void Match( const TWordIndex wordIndex );

private:
	void match( const TStateIndex stateIndex,
		const TWordIndex wordIndex );
};

void CContext::Match( const TWordIndex wordIndex )
{
	match( 0, wordIndex );
}

void CContext::match( const TStateIndex stateIndex,
	const TWordIndex wordIndex )
{
	const CState& state = states[stateIndex];
	const CTransitions& transitions = state.Transitions;

	CUndoActions undoActions;

	if( !state.Actions.Do( text, matchings, undoActions ) // conditions are not met
		|| ( transitions.empty() && state.EpsTransitions.empty() )
		|| !text.HasWord( wordIndex ) )
	{
		return;
	}

	matchings.emplace_back();
	CMatching& matching = matchings.back();
	matching.WordIndex = wordIndex;
	for( const CTransition& transition : transitions ) {
		if( transition.Match( text[wordIndex], matching.WordValueIndices ) ) {
			matching.PatternElementIndex = transition.PatternElementIndex;
			match( transition.NextState, text.NextWord( wordIndex ) );
		}
	}
	matchings.pop_back();

	for( const TStateIndex nextStateIndex : state.EpsTransitions ) {
		match( nextStateIndex, wordIndex );
	}
}

#pragma region CAgreementAction
class CAgreementUndoAction : public IUndoAction {
public:
	CAgreementUndoAction( CWordValueIndices& target,
			CWordValueIndices&& source ) :
		targetWordValueIndicies( target ),
		sourceWordValueIndicies( source )
	{
	}

	virtual void Undo() const override;

private:
	CWordValueIndices& targetWordValueIndicies;
	CWordValueIndices sourceWordValueIndicies;
};

void CAgreementUndoAction::Undo() const
{
	targetWordValueIndicies = move( sourceWordValueIndicies );
}

struct CAgreementAction : public IAction {
public:
	virtual bool Do( const CText& text, CMatchings& matchings,
		CUndoActions& undoActions ) const override;

private:
	bool strong;
	TPatternElementIndex firstElementIndex;
	TPatternElementIndex secondElementIndex;

	bool match( const CText& text,
		CMatching& first, CMatching& second,
		CUndoActions& undoActions ) const;
};

bool CAgreementAction::Do( const CText& text, CMatchings& matchings,
	CUndoActions& undoActions ) const
{
	CMatchings::reverse_iterator firstElement = matchings.rbegin();
	while( firstElement != matchings.rend()
		&& firstElement->PatternElementIndex != firstElementIndex
		&& firstElement->PatternElementIndex != secondElementIndex )
	{
		++firstElement;
	}

	if( firstElement == matchings.rend() ) {
		return true;
	}

	const TPatternElementIndex secondElementsIndex =
		firstElement->PatternElementIndex == firstElementIndex ?
			secondElementIndex : firstElementIndex;
	
	for( auto secondElement = next( firstElement );
		secondElement != matchings.rend(); ++secondElement )
	{
		if( secondElement->PatternElementIndex == secondElementsIndex ) {
			if( !match( text, *firstElement, *secondElement, undoActions ) ) {
				return false;
			}
		}
	}

	return true;
}

bool CAgreementAction::match( const CText& text,
	CMatching& first, CMatching& second,
	CUndoActions& undoActions ) const
{
	cout << "Match('" << text[first.WordIndex].text
		<< ",'" << text[second.WordIndex].text << ");";

	const CArgreement& agreement = text.Argreements().Agreement(
		make_pair( first.WordIndex, second.WordIndex ), strong );

	auto undo1 = first.WordValueIndices.Intersect( agreement.first );
	auto undo2 = second.WordValueIndices.Intersect( agreement.second );

	undoActions.Add( new CAgreementUndoAction(
		first.WordValueIndices, move( undo1 ) ) );
	undoActions.Add( new CAgreementUndoAction(
		second.WordValueIndices, move( undo2 ) ) );

	return ( first.WordValueIndices.Filled()
		&& second.WordValueIndices.Filled() );
}
#pragma endregion

#pragma region CDictionaryAction
struct CDictionaryAction : public IAction {
public:
	const string Name;

	explicit CDictionaryAction( const string& name ) :
		Name( name )
	{
	}

	void Add( const vector<TPatternElementIndex>& word )
	{
		elements.push_back( word );
	}

	virtual bool Do( const CText& text, CMatchings& matchings,
		CUndoActions& undoActions ) const override;

private:
	vector<vector<TPatternElementIndex>> elements;
};

bool CDictionaryAction::Do( const CText& text, CMatchings& matchings,
	CUndoActions& /* undoActions */ ) const
{
	unordered_multimap<TPatternElementIndex, TWordIndex> elementToWordIndex;
	for( const CMatching& matching : matchings ) {
		elementToWordIndex.emplace( matching.PatternElementIndex, matching.WordIndex );
	}

	vector<string> words;
	words.reserve( elements.size() );
	for( const vector<TPatternElementIndex>& wordElements : elements ) {
		words.emplace_back();
		for( const TPatternElementIndex wordElement : wordElements ) {
			auto range = elementToWordIndex.equal_range( wordElement );
			for( auto i = range.first; i != range.second; ++i ) {
				words.back() += text[i->second].text + " ";
			}
		}
		if( !words.back().empty() ) {
			words.back().pop_back();
		}
	}

	// check
	cout << "Dictionary('" << Name << "'";
	for( const string& word : words ) {
		cout << "," << word;
	}
	cout << ");" << endl;

	return true;
}
#pragma endregion

///////////////////////////////////////////////////////////////////////////////

void test()
{
	CText text;
	for( int i = 0; i < 10; i++ ) {
		CWord word;
		word.text = "word#" + to_string( i );
		word.word = word.text;
		CWordValue wordValue;
		wordValue.Value = "W";
		word.values.push_back( wordValue );
		wordValue.Value = "A";
		word.values.push_back( wordValue );
		wordValue.Value = "N";
		word.values.push_back( wordValue );
		text.push_back( word );
	}

	CTransition a, n, w;
	a.valueRegex.reset( new RegexEx( StringEx( "A" ) ) );
	a.PatternElementIndex = 1;
	n.valueRegex.reset( new RegexEx( StringEx( "N" ) ) );
	n.PatternElementIndex = 2;
	w.valueRegex.reset( new RegexEx( StringEx( "W" ) ) );
	w.PatternElementIndex = 3;

	CStates states;
	{ // s0
		CState state;
		state.Transitions.push_back( n );
		state.Transitions.back().PatternElementIndex = 0;
		state.Transitions.back().NextState = 1;
		state.EpsTransitions.push_back( 1 );
		states.push_back( state );
	}
	{ // s1
		CState state;
		state.Transitions.push_back( a );
		state.Transitions.back().NextState = 2;
		state.Transitions.push_back( n );
		state.Transitions.back().NextState = 3;
		state.Transitions.push_back( w );
		state.Transitions.back().NextState = 4;
		states.push_back( state );
	}
	{ // s2
		CState state;
		state.Transitions.push_back( n );
		state.Transitions.back().NextState = 5;
		state.Transitions.push_back( w );
		state.Transitions.back().NextState = 6;
		states.push_back( state );
	}
	{ // s3
		CState state;
		state.Transitions.push_back( w );
		state.Transitions.back().NextState = 7;
		states.push_back( state );
	}
	{ // s4
		CState state;
		state.Transitions.push_back( n );
		state.Transitions.back().NextState = 7;
		states.push_back( state );
	}
	{ // s5
		CState state;
		state.Transitions.push_back( w );
		state.Transitions.back().NextState = 8;
		states.push_back( state );
	}
	{ // s6
		CState state;
		state.Transitions.push_back( n );
		state.Transitions.back().NextState = 8;
		states.push_back( state );
	}
	{ // s7
		CState state;
		state.Transitions.push_back( a );
		state.Transitions.back().NextState = 8;
		states.push_back( state );
	}
	{ // s8
		CState state;
		shared_ptr<CDictionaryAction> action( new CDictionaryAction( "svetlana" ) );
		action->Add( { 0 } );
		action->Add( { 1 } );
		action->Add( { 2 } );
		action->Add( { 3 } );
		state.Actions.Add( action );
		states.push_back( state );
	}

	CContext context( text, states );
	context.Match( 0 );
}

///////////////////////////////////////////////////////////////////////////////

//} // end of Matching namespace
//} // end of Lspl namespace
