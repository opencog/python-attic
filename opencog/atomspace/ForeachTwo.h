/**
 * ForeachTwo.h
 *
 * Basic pattern-matching iterator constructs.
 *
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#ifndef _OPENCOG_FOREACH_TWO_H
#define _OPENCOG_FOREACH_TWO_H

#include <opencog/atomspace/AtomSpace.h>

namespace opencog
{

/**
 * Invoke the callback on each pair of corresponding atoms in
 * the outgoing sets of handles ha and hb. This iterator is
 * typically useful for making comparisons between atoms.
 */
template<class T>
inline bool foreach_outgoing_atom_pair(Handle ha, Handle hb,
                                       bool (T::*cb)(Handle, Handle), T *data)
{
    const AtomSpace *as = data->get_atomspace();
    if (!as->isLink(ha) || !as->isLink(hb)) return false;
    const std::vector<Handle> &va = as->getOutgoing(ha);
    const std::vector<Handle> &vb = as->getOutgoing(hb);

    size_t vasz = va.size();
    size_t vbsz = vb.size();
    size_t minsz = std::min(vasz, vbsz);

    for (size_t i = 0; i < minsz; i++) {
        ha = va[i];
        hb = vb[i];
        bool rc = (data->*cb)(ha, hb);
        if (rc) return rc;
    }

    for (size_t i = vasz; i < vbsz; i++) {
        hb = vb[i];
        bool rc = (data->*cb)(Handle::UNDEFINED, hb);
        if (rc) return rc;
    }
    for (size_t i = vbsz; i < vasz; i++) {
        ha = va[i];
        bool rc = (data->*cb)(ha,Handle::UNDEFINED);
        if (rc) return rc;
    }
    return false;
}

/**
 * Invoke the callback on each combination of atoms in
 * the outgoing sets of handles ha and hb. This iterator is
 * typically useful for making comparisons between atoms.
 */
template<class T>
inline bool foreach_outgoing_atom_combination(Handle ha, Handle hb,
                                              bool (T::*cb)(Handle, Handle), 
                                              T *data)
{
    const AtomSpace *as = data->get_atomspace();
    if (!as->isLink(ha) || !as->isLink(hb)) return false;

    // Enumerate all the permutations of the outgoing of ha
    std::vector<Handle> va = as->getOutgoing(ha);
    const std::vector<Handle> &vb = as->getOutgoing(hb);

    size_t vasz = va.size();
    size_t vbsz = vb.size();
    size_t minsz = std::min(vasz, vbsz);

    sort(va.begin(), va.end()); 

    do {
        for (size_t i = 0; i < minsz; i++) {
            ha = va[i];
            hb = vb[i];
            bool rc = (data->*cb)(ha, hb);
            if (rc) continue;
        }

        for (size_t i = vasz; i < vbsz; i++) {
            hb = vb[i];
            bool rc = (data->*cb)(Handle::UNDEFINED, hb);
            if (rc) continue;
        }

        for (size_t i = vbsz; i < vasz; i++) {
            ha = va[i];
            bool rc = (data->*cb)(ha,Handle::UNDEFINED);
            if (rc) continue; 
        }


    } while ( next_permutation(va.begin(), va.end()) ); 

    return false; 
}

} // namespace opencog

#endif // _OPENCOG_FOREACH_TWO_H
