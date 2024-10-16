/*******************************************************************************
Copyright (c) 2001 by Text Analysis International, Inc.
All rights reserved.  No part of this document may be copied, used, or
modified without written permission from Text Analysis International, Inc.
********************************************************************************
*
* NAME:	PHASH.CPP
* FILE:	lite/phash.cpp
* CR:		05/02/00 AM.
* SUBJ:	Hash elts and rules that have been internalized.
* ASS:	A pass like NINTERN has been run previous to this.
*
*******************************************************************************/

#include "StdAfx.h"
#include "machine.h"	// 10/25/06 AM.
#include "u_out.h"		// 01/19/06 AM.
#include "lite/global.h"
#include "inline.h"
#include "dlist.h"			// 02/26/01 AM.
#include "node.h"	// 07/07/03 AM.
#include "tree.h"				// 02/26/01 AM.
#include "pn.h"				// 02/14/01 AM.
#include "io.h"	// Linux.	// 04/26/07 AM.
#include "nlppp.h"	// Linux.	// 04/26/07 AM.
#include "gen.h"	// Linux.	// 04/26/07 AM.
#include "parse.h"
#include "seqn.h"
#include "ifile.h"						// 02/27/01 AM.
#include "rfasem.h"
#include "phash.h"

// For pretty printing the algorithm name.
static _TCHAR algo_name[] = _T("phash");

/********************************************
* FN:		Special Functions for class.
* CR:		05/02/00 AM.
********************************************/

Phash::Phash()			// Default constructor
	: Algo(algo_name /*, 0*/)
{

}


Phash::Phash(const Phash &orig)			// Copy constructor
{
name = orig.name;
debug_ = orig.debug_;
}

/********************************************
* FN:		Access Functions
********************************************/



/********************************************
* FN:		Modify Functions
********************************************/




/********************************************
* FN:		SETUP
* CR:		05/02/00 AM.
* SUBJ:	Set up Algo as an analyzer pass.
* ARG:	s_data = This is an argument from the analyzer sequence file,
*		   if any, for the current pass.
********************************************/

void Phash::setup(_TCHAR *s_data)
{
// No arguments to this pass in sequence file.
}

/********************************************
* FN:		DUP
* CR:		05/02/00 AM.
* SUBJ:	Duplicate the given Algo object.
* NOTE:	Don't know a better way to have a base pointer duplicate the
*			object that it points to!
********************************************/

Algo &Phash::dup()
{
Phash *ptr;
ptr = new Phash(*this);					// Copy constructor.
//ptr = new Pat();
//ptr->setName(this->name);
//ptr->setParse(this->parse);
return (Algo &) *ptr;
}

/********************************************
* FN:		EXECUTE
* CR:		05/02/00 AM.
* SUBJ:	Perform the traversal and interning.
* NOTE:	Now, just picking the rules right off the tree!
*			Use this pass to internalize the contents of the pairs lists.
********************************************/

bool Phash::Execute(Parse *parse, Seqn *seqn)
{
if (parse->Verbose())
	*gout << _T("[Phash:]") << endl;

Tree<Pn> *tree = (Tree<Pn> *) parse->getTree();

if (!tree)
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: No parse tree.]") << ends;
		return errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

Node<Pn> *root;
root = tree->getRoot();

if (!root)
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: Empty parse tree.]") << ends;
		return errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

// Get the rules file node, if any.
Node<Pn> *node;
if (!(node = root->Down()))
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: Rules file '")
		  // << str(seqn->getRulesfilename())
			  << str(parse->getInput())
			  << _T("' not parsed.]") << ends;
		errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

if (node->Right())
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Pass file has some unhandled text.]") << ends;
		errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	}

Pn *pn;
pn = node->getData();
assert(pn);
RFASem *sem;
sem = (RFASem *) pn->getSem();
if (!sem)
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: No semantics for parse tree.]") << ends;
		errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

// Could search for a _RULESFILE node at the top level here.
if (sem->getType() != RSFILE)
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: Couldn't parse rules file.]") << ends;
		errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

Ifile *rulesfile;
if (!(rulesfile = sem->getRulesfile()))
	{
	if (parse->getIntern())													// 05/26/01 AM.
		{
		parse->setIntern(false);	// Syntax error.					// 05/26/01 AM.
		_t_strstream gerrStr;
		gerrStr << _T("[Phash: No rules found in file.]") << ends;
		errOut(&gerrStr,false,parse->getInputpass(),0);
		}
	return false;
	}

Dlist<Irule> *rules = rulesfile->getRules();

if (!rules)
	{
	// Got to trust that if there's a problem, we'll be
	// issuing at least one error message.								// 05/26/01 AM.
//	if (parse->getIntern())													// 05/26/01 AM.
//		{
//		strstream gerrStr(Errbuf,MAXSTR,ios::out);
//		gerrStr << "[Warning: No rules in pass file.]" << ends;
//		errOut(true,parse->getInputpass(),0);
//		}
	return true;
	}

// HASH THE ELTS OF EVERY RULE.
Irule::ehash(rules);


// HASH ALL THE RULES OF THE FILE.
rulesfile->rhash(
					parse);														// 04/03/03 AM.

// CLEAN THE MATCH/FAIL LISTS OF RULES.
Irule::pruneLists(rules);

return true;
}


/******************************************************************************/
