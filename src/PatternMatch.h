#pragma once

#include <Text.h>
#include <Configuration.h>
#include <FixedSizeArray.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

typedef uint8_t TVariantSize;
const TVariantSize MaxVariantSize = numeric_limits<TVariantSize>::max();

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

///////////////////////////////////////////////////////////////////////////////

class CBaseTransition {
public:
	explicit CBaseTransition( const TStateIndex nextState );
	virtual ~CBaseTransition() = 0 {}

	const TStateIndex NextState() const { return nextState; }
	virtual bool Match( const Text::CWord& word,
		/* out */ Text::CAnnotationIndices& indices ) const = 0;

private:
	const TStateIndex nextState;
};

typedef unique_ptr<CBaseTransition> CTransitionPtr;
typedef vector<CTransitionPtr> CTransitions;

///////////////////////////////////////////////////////////////////////////////

class CWordTransition : public CBaseTransition {
public:
	CWordTransition( Text::RegexEx&& wordRegex, const TStateIndex nextState );
	~CWordTransition() override {}

	bool Match( const Text::CWord& word,
		/* out */ Text::CAnnotationIndices& indices ) const override;

private:
	const Text::RegexEx wordRegex;
};

///////////////////////////////////////////////////////////////////////////////

class CAttributesTransition : public CBaseTransition {
public:
	CAttributesTransition( Text::CAttributesRestriction&& attributesRestriction,
		const TStateIndex nextState );
	~CAttributesTransition() override {}

	bool Match( const Text::CWord& word,
		/* out */ Text::CAnnotationIndices& indices ) const override;

private:
	const Text::CAttributesRestriction attributesRestriction;
};

///////////////////////////////////////////////////////////////////////////////

class CMatchContext;

class IAction {
public:
	virtual ~IAction() = 0 {}
	virtual bool Run( const CMatchContext& context ) const = 0;
	virtual void Print( const Configuration::CConfiguration& configuration,
		ostream& out ) const = 0;
};

typedef shared_ptr<IAction> CActionPtr;

///////////////////////////////////////////////////////////////////////////////

class CActions {
public:
	CActions() = default;
	void Add( CActionPtr action );
	bool Run( const CMatchContext& context ) const;
	void Print( const Configuration::CConfiguration& configuration,
		ostream& out ) const;

private:
	vector<CActionPtr> actions;
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

	const Text::CText& Text() const { return text; }
	const CDataEditor& DataEditor() const;
	const Text::TWordIndex InitialWord() const { return initialWordIndex; }
	const Text::TWordIndex Shift() const { return data.size(); }
	const Text::TWordIndex Word() const;
	void Match( const Text::TWordIndex initialWordIndex );

private:
	const Text::CText& text;
	const CStates& states;
	Text::TWordIndex initialWordIndex;
	vector<Text::CAnnotationIndices> data;
	stack<CDataEditor> editors;

	void match( const TStateIndex stateIndex );
};

///////////////////////////////////////////////////////////////////////////////

class CAgreementAction : public IAction {
public:
	CAgreementAction( const Text::TAttribute attribute,
		const TVariantSize offset );
	CAgreementAction( const Text::TAttribute attribute,
		const TVariantSize offset, const vector<TVariantSize>& words );

	~CAgreementAction() override {}
	bool Run( const CMatchContext& context ) const override;
	void Print( const Configuration::CConfiguration& configuration,
		ostream& out ) const override;

private:
	const bool strong;
	const Text::TAttribute attribute;
	CFixedSizeArray<TVariantSize, TVariantSize> offsets;
};

///////////////////////////////////////////////////////////////////////////////

class CDictionaryAction : public IAction {
public:
	CDictionaryAction( const Configuration::TDictionary dictionary,
		const TVariantSize offset, const vector<TVariantSize>& words );

	~CDictionaryAction() override {}
	bool Run( const CMatchContext& context ) const override;
	void Print( const Configuration::CConfiguration& configuration,
		ostream& out ) const override;

private:
	const Configuration::TDictionary dictionary;
	CFixedSizeArray<TVariantSize, TVariantSize> offsets;
};

///////////////////////////////////////////////////////////////////////////////

class CPrintAction : public IAction {
public:
	explicit CPrintAction( ostream& out );
	~CPrintAction() override {}
	bool Run( const CMatchContext& context ) const override;
	void Print( const Configuration::CConfiguration& configuration,
		ostream& out ) const override;

private:
	ostream& output;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
