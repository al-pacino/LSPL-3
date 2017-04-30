#pragma once

#include <common.h>
#include <PatternMatch.h>

using namespace Lspl::Text;
using namespace Lspl::Configuration;

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

void CEdges::AddEdge( const CDataEditor& graph,
	const TVariantSize word1, const TAnnotationIndex index1,
	const TVariantSize word2, const TAnnotationIndex index2,
	const TAttribute attribute )
{
	CEdges& data1 = graph.GetForEdit( word1 );
	debug_check_logic( data1.Indices.Has( index1 ) );
	const auto pair1 = data1.EdgeSet.insert( { index1, word2, attribute, index2 } );
	debug_check_logic( pair1.second );

	CEdges& data2 = graph.GetForEdit( word2 );
	debug_check_logic( data2.Indices.Has( index2 ) );
	const auto pair2 = data2.EdgeSet.insert( { index2, word1, attribute, index1 } );
	debug_check_logic( pair2.second );
}

bool CEdges::RemoveEdge( const CDataEditor& graph,
	const TVariantSize word1, const TAnnotationIndex index1,
	const TVariantSize word2, const TAnnotationIndex index2,
	const TAttribute attribute )
{
	CEdges& data = graph.GetForEdit( word1 );
	CEdgeSet& edges = data.EdgeSet;
	const CEdgeSet::iterator e = edges.find( { index1, word2, attribute, index2 } );
	if( e == edges.end() ) {
		return true;
	}

	bool removeVertex = true;
	if( e != edges.begin() ) {
		auto be = prev( e );
		if( be->Index1 == e->Index1
			&& be->Word2 == e->Word2
			&& be->Attribute == e->Attribute )
		{
			removeVertex = false;
		}
	}
	if( removeVertex ) {
		auto ae = next( e );
		if( ae != edges.end()
			&& ae->Index1 == e->Index1
			&& ae->Word2 == e->Word2
			&& ae->Attribute == e->Attribute )
		{
			removeVertex = false;
		}
	}
	edges.erase( e );
	if( removeVertex ) {
		return RemoveVertex( graph, word1, index1 );
	}
	return true;
}

bool CEdges::RemoveVertex( const CDataEditor& graph,
	const TVariantSize word1, const TAnnotationIndex index1 )
{
	CEdges& data = graph.GetForEdit( word1 );
	CEdgeSet& edges = data.EdgeSet;
	if( !data.Indices.Has( index1 ) ) {
		return true;
	}

	struct {
		bool operator()( const CEdge& edge, const TAnnotationIndex index ) const
		{
			return ( edge.Index1 < index );
		}
	} comp;
	const CEdgeSet::iterator e = lower_bound( edges.begin(), edges.end(), index1, comp );

	const bool erased = data.Indices.Erase( index1 );
	debug_check_logic( erased );

	if( data.Indices.IsEmpty() ) {
		return false;
	}

	CEdgeSet::iterator j = e;
	while( j != edges.end() && j->Index1 == index1 ) {
		RemoveEdge( graph, j->Word2, j->Index2, word1, j->Index1, j->Attribute );
		++j;
	}
	edges.erase( e, j );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

CDataEditor::CDataEditor( CData& _data ) :
	data( _data )
{
}

CDataEditor::~CDataEditor()
{
	Restore();
}

const CData::value_type& CDataEditor::Get( const CData::size_type index ) const
{
	debug_check_logic( index < data.size() );
	return data[index];
}

CData::value_type& CDataEditor::GetForEdit( const CData::size_type index ) const
{
	debug_check_logic( index < data.size() );
	if( ( index + 1 ) < data.size() ) { // do not save last element
		dump.insert( make_pair( index, data[index] ) );
	}
	return data[index];
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

const TVariantSize CMatchContext::Shift() const
{
	debug_check_logic( !data.empty() );
	return Cast<TVariantSize>( data.size() - 1 );
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
		if( transition->Match( Text().Word( Word() ), data.back().Indices ) ) {
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
	const TVariantSize word2 = context.Shift();
	for( TVariantSize i = 0; i < offsets.Size(); i++ ) {
		debug_check_logic( offsets[i] > 0 );
		debug_check_logic( offsets[i] <= word2 );
		if( !agree( context, word2 - offsets[i], word2 ) ) {
			return false;
		}
	}
	return true;
}

bool CAgreementAction::agree( const CMatchContext& context,
	const TVariantSize word1, const TVariantSize word2 ) const
{
	debug_check_logic( word1 < word2 );

	const CDataEditor& editor = context.DataEditor();
	CEdges& edges1 = editor.GetForEdit( word1 );
	CEdges& edges2 = editor.GetForEdit( word2 );

	const CAnnotations wa1
		= context.Text().Word( context.InitialWord() + word1 ).Annotations();
	const CAnnotations wa2
		= context.Text().Word( context.InitialWord() + word2 ).Annotations();

	bool added = false;
	CAnnotationIndices unused1 = edges1.Indices;
	CAnnotationIndices unused2 = edges2.Indices;

	for( CAnnotationIndices::SizeType i1 = 0; i1 < edges1.Indices.Size(); i1++ ) {
		for( CAnnotationIndices::SizeType i2 = 0; i2 < edges2.Indices.Size(); i2++ ) {
			const TAnnotationIndex index1 = edges1.Indices.Value( i1 );
			const TAnnotationIndex index2 = edges2.Indices.Value( i2 );

			switch( wa1[index1].Agreement( wa2[index2], attribute ) ) {
				case AP_None:
					continue;
				case AP_Strong:
					break;
				case AP_Weak:
					if( strong ) {
						continue;
					}
					break;
			}
			added = true;
			unused1.Erase( index1 );
			unused2.Erase( index2 );
			CEdges::AddEdge( editor, word1, index1, word2, index2, attribute );
		}
	}

	if( !added ) {
		return false;
	}

	for( TAnnotationIndex i = 0; i < unused1.Size(); i++ ) {
		if( !CEdges::RemoveVertex( editor, word1, unused1.Value( i ) ) ) {
			return false;
		}
	}
	for( TAnnotationIndex i = 0; i < unused2.Size(); i++ ) {
		if( !CEdges::RemoveVertex( editor, word2, unused2.Value( i ) ) ) {
			return false;
		}
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
