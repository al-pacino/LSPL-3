#pragma once

#include <common.h>
#include <PatternMatch.h>

using namespace Lspl::Text;
using namespace Lspl::Configuration;

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

CDataEditor::CDataEditor( CData& _data ) :
	data( _data )
{
}

CDataEditor::~CDataEditor()
{
	Restore();
}

const CData::value_type& CDataEditor::Value( const CData::size_type index ) const
{
	debug_check_logic( index < data.size() );
	return data[index];
}

void CDataEditor::Set( const CData::size_type index,
	CData::value_type&& value ) const
{
	debug_check_logic( index < data.size() );
	dump.insert( make_pair( index, data[index] ) );
	data[index] = value;
}

void CDataEditor::Restore()
{
	for( pair<const CData::size_type, CData::value_type>& key : dump ) {
		debug_check_logic( key.first < data.size() );
		data[key.first] = move( key.second );
	}
}

///////////////////////////////////////////////////////////////////////////////

CBaseTransition::CBaseTransition( const TStateIndex _nextState ) :
	nextState( _nextState )
{
	debug_check_logic( nextState > 0 );
}

///////////////////////////////////////////////////////////////////////////////

CWordTransition::CWordTransition( RegexEx&& _wordRegex,
		const TStateIndex nextState ) :
	CBaseTransition( nextState ),
	wordRegex( move( _wordRegex ) )
{
}

bool CWordTransition::Match( const CWord& word,
	CAnnotationIndices& indices ) const
{
	if( !word.MatchWord( wordRegex ) ) {
		return false;
	}
	indices = move( word.AnnotationIndices() );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

CAttributesTransition::CAttributesTransition(
		CAttributesRestriction&& _attributesRestriction,
		const TStateIndex nextState ) :
	CBaseTransition( nextState ),
	attributesRestriction( move( _attributesRestriction ) )
{
	debug_check_logic( !attributesRestriction.IsEmpty() );
}

bool CAttributesTransition::Match( const CWord& word,
	CAnnotationIndices& indices ) const
{
	return word.MatchAttributes( attributesRestriction, indices );
}

///////////////////////////////////////////////////////////////////////////////

void CActions::Add( CActionPtr action )
{
	actions.emplace_back( action );
}

bool CActions::Run( const CMatchContext& context ) const
{
	for( const shared_ptr<IAction>& action : actions ) {
		if( !action->Run( context ) ) {
			return false;
		}
	}
	return true;
}

void CActions::Print( const CConfiguration& configuration, ostream& out ) const
{
	for( const shared_ptr<IAction>& action : actions ) {
		action->Print( configuration, out );
	}
}

///////////////////////////////////////////////////////////////////////////////

CMatchContext::CMatchContext( const CText& text, const CStates& states ) :
	text( text ),
	states( states ),
	initialWordIndex( 0 )
{
	data.reserve( 32 );
}

const CDataEditor& CMatchContext::DataEditor() const
{
	debug_check_logic( !editors.empty() );
	return editors.top();
}

const Text::TWordIndex CMatchContext::Shift() const
{
	debug_check_logic( !data.empty() );
	return ( data.size() - 1 );
}

const Text::TWordIndex CMatchContext::Word() const
{
	return ( InitialWord() + Shift() );
}

void CMatchContext::Match( const TWordIndex _initialWordIndex )
{
	debug_check_logic( data.empty() );
	debug_check_logic( editors.empty() );
	initialWordIndex = _initialWordIndex;
	match( 0 );
}

void CMatchContext::match( const TStateIndex stateIndex )
{
	const CState& state = states[stateIndex];
	const CTransitions& transitions = state.Transitions;

	if( !state.Actions.Run( *this ) // conditions are not met
		|| transitions.empty() // leaf
		|| !( ( InitialWord() + data.size() ) < Text().Length() ) )
	{
		return;
	}

	data.emplace_back();
	editors.emplace( data );
	for( const CTransitionPtr& transition : transitions ) {
		if( transition->Match( Text().Word( Word() ), data.back() ) ) {
			match( transition->NextState() );
		}
	}
	editors.pop();
	data.pop_back();
}

///////////////////////////////////////////////////////////////////////////////

CAgreementAction::CAgreementAction( const TAttribute _attribute,
		const TVariantSize offset ) :
	strong( true ),
	attribute( _attribute ),
	offsets( 1 )
{
	offsets[0] = offset;
}

CAgreementAction::CAgreementAction( const TAttribute _attribute,
		const TVariantSize offset, const vector<TVariantSize>& words ) :
	strong( false ),
	attribute( _attribute ),
	offsets( Cast<TVariantSize>( words.size() ) )
{
	debug_check_logic( !words.empty() );
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		debug_check_logic( words[i] <= offset );
		offsets[i] = offset - words[i];
	}
}

bool CAgreementAction::Run( const CMatchContext& context ) const
{
	const CDataEditor& editor = context.DataEditor();
	const CArgreements& agreements = context.Text().Argreements();
	const TWordIndex index2 = context.Shift();
	const CAnnotationIndices& indices2 = editor.Value( index2 );

	CArgreements::CKey key{ { 0, context.Word() }, attribute };
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		const TVariantSize offset = offsets[i];
		debug_check_logic( offset <= index2 );
		const TWordIndex index1 = index2 - offset;
		const CAnnotationIndices& indices1 = editor.Value( index1 );
		key.first.first = context.Word() - offset;
		CAgreement agr = agreements.Agreement( key, strong );
		agr.first = CAnnotationIndices::Intersection( agr.first, indices1 );
		agr.second = CAnnotationIndices::Intersection( agr.second, indices2 );

		if( agr.first.IsEmpty() || agr.second.IsEmpty() ) {
			return false;
		}

		editor.Set( index1, move( agr.first ) );
		editor.Set( index2, move( agr.second ) );
	}
	return true;
}

void CAgreementAction::Print( const CConfiguration& configuration,
	ostream& out ) const
{
	out << "<<"
		<< configuration.Attributes()[attribute].Names.Value( 0 )
		<< ( strong ? "==" : "=" );
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		if( i > 0 ) {
			out << ",";
		}
		out << Cast<uint32_t>( offsets[i] );
	}
	out << ">>";
}

///////////////////////////////////////////////////////////////////////////////

CDictionaryAction::CDictionaryAction( const TDictionary _dictionary,
		const TVariantSize offset, const vector<TVariantSize>& words ) :
	dictionary( _dictionary ),
	offsets( Cast<TVariantSize>( words.size() ) )
{
	debug_check_logic( !words.empty() );
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		if( words[i] < MaxVariantSize ) {
			debug_check_logic( words[i] <= offset );
			offsets[i] = offset - words[i];
		} else {
			offsets[i] = MaxVariantSize;
		}
	}
}

bool CDictionaryAction::Run( const CMatchContext& context ) const
{
	vector<string> words;
	words.emplace_back();
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		const TVariantSize offset = offsets[i];
		if( offset == MaxVariantSize ) {
			debug_check_logic( !words.back().empty() );
			words.back().pop_back();
			words.emplace_back();
		} else {
			debug_check_logic( offset <= context.Shift() );
			const TWordIndex word = context.Word() - offset;
			words.back() += context.Text().Word( word ).text + " ";
		}
	}
	debug_check_logic( !words.back().empty() );
	words.back().pop_back();

#ifdef _DEBUG
	cout << "dictionary{" << dictionary << "}(";
	bool first = true;
	for( const string& word : words ) {
		if( first ) {
			first = false;
		} else {
			cout << ",";
		}
		cout << word;
	}
	cout << ");" << endl;
#endif

	return true;
}

void CDictionaryAction::Print(
	const Configuration::CConfiguration& /*configuration*/,
	ostream& out ) const
{
	out << "<<" << dictionary << "(";
	bool first = true;
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		if( first ) {
			first = false;
		} else {
			out << " ";
		}
		if( offsets[i] == MaxVariantSize ) {
			out << ",";
			first = true;
		} else {
			out << Cast<uint32_t>( offsets[i] );
		}
	}
	out << ")>>";
}

///////////////////////////////////////////////////////////////////////////////

CPrintAction::CPrintAction( ostream& out ) :
	output( out )
{
}

bool CPrintAction::Run( const CMatchContext& context ) const
{
	const TWordIndex begin = context.InitialWord();
	const TWordIndex end = context.Word();

	output << "{";
	for( TWordIndex wi = begin; wi < end; wi++ ) {
		output << context.Text().Word( wi ).text << " ";
	}
	output << context.Text().Word( end ).text << "}" << endl;

	return true;
}

void CPrintAction::Print( const Configuration::CConfiguration& /*configuration*/,
	ostream& out ) const
{
	out << "<<Save>>";
}

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
