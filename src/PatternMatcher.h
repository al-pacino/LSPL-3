#pragma once

#include <Text.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

typedef size_t TSign;

///////////////////////////////////////////////////////////////////////////////

template<class TYPE, class SOURCE_TYPE>
inline const TYPE Cast( const SOURCE_TYPE sourceValue )
{
	// only numeric types are allowed
	static_assert( TYPE() * 2 == SOURCE_TYPE() * 2, "bad cast" );
	const TYPE value = static_cast<TYPE>( sourceValue );
	debug_check_logic( static_cast<SOURCE_TYPE>( value ) == sourceValue );
	debug_check_logic( ( value >= 0 ) == ( sourceValue >= 0 ) );
	return value;
}

///////////////////////////////////////////////////////////////////////////////

struct CPatternWordCondition {
	typedef uint8_t TValue;
	static const TValue Max = numeric_limits<TValue>::max();

	CPatternWordCondition( const TValue offset, const TSign param );
	CPatternWordCondition( const TValue offset,
		const vector<TValue>& words, const TSign param );
	CPatternWordCondition( const CPatternWordCondition& another );
	CPatternWordCondition& operator=( const CPatternWordCondition& another );
	CPatternWordCondition( CPatternWordCondition&& another );
	CPatternWordCondition& operator=( CPatternWordCondition&& another );
	~CPatternWordCondition();

	void Print( ostream& out ) const;

	TValue Size;
	bool Strong;
	TSign Param;
	TValue* Offsets;
};

///////////////////////////////////////////////////////////////////////////////

typedef vector<Text::CAnnotationIndices> CData;

class CDataEditor {
public:
	explicit CDataEditor( CData& data );
	~CDataEditor();

	const CData::value_type& Value( const CData::size_type index ) const;
	void Set( const CData::size_type index,
		CData::value_type&& value ) const;
	void Restore();

private:
	CData& data;
	mutable unordered_map<CData::size_type, CData::value_type> dump;
};

///////////////////////////////////////////////////////////////////////////////

struct CState;
typedef vector<CState> CStates;
typedef CStates::size_type TStateIndex;

struct CTransition {
	bool Word;
	TStateIndex NextState;
	RegexEx WordOrAttributesRegex;

	bool Match( const Text::CWord& word,
		/* out */ Text::CAnnotationIndices& indices ) const;
};

typedef vector<CTransition> CTransitions;

///////////////////////////////////////////////////////////////////////////////

class CMatchContext;

class IAction {
public:
	virtual ~IAction() = 0 {}
	virtual bool Run( const CMatchContext& context ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class CActions {
public:
	void Add( shared_ptr<IAction> action );
	bool Run( const CMatchContext& context ) const;

private:
	vector<shared_ptr<IAction>> actions;
};

///////////////////////////////////////////////////////////////////////////////

struct CState {
	CActions Actions;
	CTransitions Transitions;
};

///////////////////////////////////////////////////////////////////////////////

class CMatchContext {
	CMatchContext( const CMatchContext& ) = delete;
	CMatchContext& operator=( const CMatchContext& ) = delete;

public:
	CMatchContext( const Text::CText& text, const CStates& states );

	const Text::TWordIndex Word() const { return wordIndex; }
	const Text::TWordIndex InitialWord() const { return initialWordIndex; }
	const Text::CText& Text() const { return text; }
	const CDataEditor& DataEditor() const { return dataEditor; }
	const Text::TWordIndex Shift() const;
	void Match( const Text::TWordIndex initialWordIndex );

private:
	const Text::CText& text;
	const CStates& states;
	Text::TWordIndex wordIndex;
	Text::TWordIndex initialWordIndex;
	vector<Text::CAnnotationIndices> data;
	CDataEditor dataEditor;

	void match( const TStateIndex stateIndex );
};

///////////////////////////////////////////////////////////////////////////////

class CAgreementAction : public IAction {
public:
	explicit CAgreementAction( const CPatternWordCondition& condition );
	~CAgreementAction() override {}
	bool Run( const CMatchContext& context ) const override;

private:
	const CPatternWordCondition condition;
};

///////////////////////////////////////////////////////////////////////////////

class CDictionaryAction : public IAction {
public:
	explicit CDictionaryAction( const CPatternWordCondition& condition );
	~CDictionaryAction() override {}
	bool Run( const CMatchContext& context ) const override;

private:
	const CPatternWordCondition condition;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
