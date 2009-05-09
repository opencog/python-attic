/*
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef QUANTIFIERRULE_H
#define QUANTIFIERRULE_H

namespace opencog { namespace pln {

template<typename FormulaType>
class QuantifierRule : public Rule
{
	pHandle domain;
	FormulaType f;
    Type OUTPUT_LINK_TYPE;
public:
	bool validate2				(MPs& args) const { return true; }

	QuantifierRule(iAtomSpaceWrapper *_destTable, const pHandle& _domain, Type outLinkType)
	: Rule(_destTable, false, true, "QuantifierRule"), 
	domain(_domain), OUTPUT_LINK_TYPE(outLinkType) {
		inputFilter.push_back(meta(
			new tree<Vertex>(
				mva((pHandle)((OUTPUT_LINK_TYPE==FORALL_LINK) ? EXIST_LINK : FORALL_LINK),
					mva((pHandle)ATOM),
					mva((pHandle)ATOM))
			)));
	}
	Rule::setOfMPs o2iMetaExtra(meta outh, bool& overrideInputFilter) const
	{
		return Rule::setOfMPs(); //No support (yet)
	}

	//Domain should be inferred instead from the premis ConceptNodes!!!

	BoundVertex compute(const vector<Vertex>& premiseArray, pHandle CX = PHANDLE_UNDEFINED) const
	{
		const int n = (int)premiseArray.size();
		TruthValue** tvs = (TruthValue**)new SimpleTruthValue*[n];
		int i;
		for (i = 0; i < n; i++)
			tvs[i] = (TruthValue*) &(GET_ATW->getTV(boost::get<pHandle>(premiseArray[i])));

		TruthValue* retTV = f.compute(tvs, n);	

		delete[] tvs;

//haxx::
		pHandle ret = destTable->addLink(OUTPUT_LINK_TYPE, pHandleSeq(),
				*retTV,
				RuleResultFreshness);	
//				false);

        delete retTV;

		return Vertex(ret);
	}
	
	NO_DIRECT_PRODUCTION;
};

typedef QuantifierRule<FORALLFormula> FORALLRule;
typedef QuantifierRule<EXISTFormula> ExistRule;
typedef QuantifierRule<PredicateTVFormula> PLNPredicateRule;

}} // namespace opencog { namespace pln {
#endif // QUANTIFIERRULE_H
