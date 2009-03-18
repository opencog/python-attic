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

#include "AtomSpaceWrapper.h"

//#include "utils/XMLNodeLoader.h"
#include "rules/Rules.h"

#include <opencog/atomspace/SimpleTruthValue.h>
#include <opencog/util/Logger.h>
#include <opencog/server/CogServer.h> // To get access to AtomSpace
#include <opencog/util/tree.h>
#include <opencog/atomspace/utils.h>
#include <opencog/atomspace/utils.h>
#include <opencog/xml/FileXMLBufferReader.h>
#include <opencog/xml/XMLBufferReader.h>
#include <opencog/xml/NMXmlParser.h>

#include  <boost/foreach.hpp>
//boost::variant<int, char> bv; //TODELETE

// Mind Shadow is an atom cache system but no longer used
#define USE_MIND_SHADOW 0
#define ARCHIVE_THEOREMS 1

// MACRO for getting the AtomSpace
// @todo: this should be replaced with initialising the ATW with a AtomSpace
// pointer/reference and using that. This should actually store an interface
// that AtomSpace provides but OpenCog currently doesn't indicate what this
// should be.
#define AS_PTR (CogServer::getAtomSpace())

using namespace std;
using namespace opencog;

// TODELETE Not used anywhere
//#ifndef USE_PSEUDOCORE
//void HandleEntry2HandleSeq(HandleEntry& src, vector<Handle>& dest)
//{ 
//    int array_size=0;
//    Handle *dest_array = new Handle[1000];
//
//    src.toHandleVector(dest_array, array_size);
//    for (int i =0;i<array_size;i++)
//    {
//        dest.push_back(dest_array[i]);
//    }
//
//    delete dest_array;
//}
//#endif
namespace haxx
{
//    reasoning::iAtomSpaceWrapper* defaultAtomSpaceWrapper;

    // TODELETE, not used
    // set<Handle> atomRemoveSet;
    
// TODELETE, only modifed, not read
//    //! childOf records what atoms are connected by inheritance links, 1st is
//    //! parent, second is child (I think...)
//    multimap<Handle,Handle> childOf;

    //! Whether to stores variable nodes as atoms in the AtomSpace
    bool AllowFW_VARIABLENODESinCore = true;

    //! ?
    map<string,pHandle> variableShadowMap;
    bool ArchiveTheorems = true;
    
#if USE_MIND_SHADOW
    vector<pHandle> mindShadow;
    map<Type, vector<pHandle> > mindShadowMap;
#endif
}

//static map<Handle,vtree> h2vtree_cache;
vtree reasoning::make_vtree(pHandle h)
{
    // Makes vtree for internal PLN use, so don't convert to real 
    // AtomSpace Handles
    
    /// @todo haxx:: Re-enable cache. It must simply be updated so that (pseudo)core reset takes it into account.
    /* map<Handle,vtree>::iterator i = h2vtree_cache.find(h);
       if (i != h2vtree_cache.end())
          return i->second;*/
    vtree ret;
    reasoning::makeHandletree(h, true, ret);
    // h2vtree_cache[h] = ret;

    reasoning::printTree(h,0,0);
    rawPrint(ret, ret.begin(), 0);

    return ret;
}

//#ifdef USE_PSEUDOCORE

float ContradictionLimit = 0.1f; //Below the limit, we just revise the differences.

//const TruthValue& AtomSpaceWrapper::getTV(Handle h)
//{
//  return AS_PTR->getTV(h);
//}

//namespace reasoning
//{
//shared_ptr<set<Handle> > AtomSpaceWrapper::getHandleSet(Type T, const string& name, bool subclass) const
//{
//    vector<Handle> hs(AS_PTR->getHandleSet(T,name,subclass));
//    return shared_ptr<set<Handle> >(new set<Handle>(hs.begin(), hs.end()));
//}
//
//Handle AtomSpaceWrapper::getHandle(Type t,const string& str) const
//{
//  AtomSpace *a = AS_PTR;
//  return AS_PTR->getHandle(t, str);
//}

//Handle AtomSpaceWrapper::getHandle(Type t,const HandleSeq& outgoing) const
//{
//  return AS_PTR->getHandle(t, outgoing);
//}

//}; //namespace reasoning

//#else

namespace reasoning
{
AtomSpaceWrapper::AtomSpaceWrapper() :
USize(800), USizeMode(CONST_SIZE)
{
    rootContext = "___PLN___";
    // Add dummy root NULL context node
    AS_PTR->addNode(CONCEPT_NODE, rootContext);

    // TODO: Replace srand with opencog::RandGen
    srand(12345678);
    linkNotifications = true;
}

void AtomSpaceWrapper::HandleEntry2HandleSet(HandleEntry& src, set<Handle>& dest) const
{   
    std::vector<Handle> dest_array;

    src.toHandleVector(dest_array);
    for (unsigned int i =0;i<dest_array.size();i++)
    {
        dest.insert(dest_array[i]);
    }
}

bool AtomSpaceWrapper::inheritsType(Type T1, Type T2)
{
    return ClassServer::isA(T1, T2);
}

bool AtomSpaceWrapper::isSubType(pHandle h, Type T)
{
    Handle r = fakeToRealHandle(h).first;
    return inheritsType(AS_PTR->getType(r), T);
}

pHandleSeq AtomSpaceWrapper::getOutgoing(const pHandle h)
{
    // Check if link
    if (!isSubType(h, LINK)) {
        // Nodes have no outgoing set
        return pHandleSeq();
    } else {
        vhpair v = fakeToRealHandle(h);
        if (v.second.substantive != Handle::UNDEFINED)
            return realToFakeHandles(v.first, v.second.substantive);
        else
            return realToFakeHandles(AS_PTR->getOutgoing(v.first));
    }
}

pHandle AtomSpaceWrapper::getOutgoing(const pHandle h, const int i) 
{
    if (i < getArity(h))
        return getOutgoing(h)[i];
    else
        cout << "no outgoing set!" << endl;
        printTree(h,0,0);
        return PHANDLE_UNDEFINED;
}

pHandleSeq AtomSpaceWrapper::getIncoming(const pHandle h) 
{
    vhpair v = fakeToRealHandle(h);
    Handle sourceContext = v.second.substantive;
    HandleSeq inLinks;
    pHandleSeq results;
    inLinks = AS_PTR->getIncoming(v.first);
    cout << "getIncoming for " << h << endl; 
    cout << "inLinks size " << inLinks.size() << endl;
    foreach (Handle h2, inLinks) {
        cout << "arity = " << AS_PTR->getArity(h2) << " h= " << h2 << endl;
    }

    // For each link in incoming, check that the context of h is
    // in the right position of the outgoing set of the link
    foreach (Handle l, inLinks) {
        // each link incoming can consist of multiple fake handles
        pHandleSeq moreLinks = realToFakeHandle(l);
        foreach (pHandle ml, moreLinks) {
            // for each fake link into h get the real handle and version handle
            // pair
            vhpair v2= fakeToRealHandle(ml);
            // check the outgoing set of the context
            HandleSeq outgoing = AS_PTR->getOutgoing(v2.first);
            if (v2.second.substantive != Handle::UNDEFINED) {
                HandleSeq contexts = AS_PTR->getOutgoing(v2.second.substantive);
                assert ((outgoing.size() + 1) == contexts.size());
                bool match = true;
                for (uint i = 0; i < outgoing.size(); i++) {
                    if (outgoing[i] == v.first) {
                        Handle c = contexts[i+1];
                        if (AS_PTR->getName(c) == rootContext)
                            c = Handle::UNDEFINED;
                        if (sourceContext != c) {
                            match = false;
                        }
                    }

                }
                if (match)
                    results.push_back(ml);
            } else {
                results.push_back(ml);
            }
        }

    }
    //return realToFakeHandles(s1, v.second);
    return results;
}

/*
bool AtomSpaceWrapper::hasAppropriateContext(const Handle o, VersionHandle& vh, unsigned int index) const
{
    if (vh == NULL_VERSION_HANDLE || !AS_PTR->getTV(o,vh).isNullTv()) {
        return true;
    } else {
        // else check immediate super/parent contexts
        HandleSeq super = AS_PTR->getOutgoing(vh.substantive);
        // check context at position i
        Handle c = super[index];
        // Root context is NULL
        if (AS_PTR->getName(c) == rootContext)
            c = UNDEFINED_HANDLE;
        VersionHandle vh2(CONTEXTUAL, c);
        if (!AS_PTR->getTV(o,vh2).isNullTv()) {
            vh.substantive = c;
            return true;
        }
    }
    return false;
}*/

/*bool AtomSpaceWrapper::isSubcontextOf(const Handle sub, const Handle super)
{
    // Handles should already be real as they are for contexts
    
    // get the name of the super
    std::string superName = AS_PTR->getName(fakeToRealHandle(super).first);

    // Check if there exists a inheritance link
    HandleEntry *he = TLB::getAtom(sub)->getNeighbors(false, true,
            INHERITANCE_LINK);
    HandleEntry::filterSet(he,superName.c_str(),CONCEPT_NODE,false); 

    bool result = he->getSize() ? true : false;
    delete he;
    return result;
}*/

vhpair AtomSpaceWrapper::fakeToRealHandle(const pHandle h) const
{
    // Don't map Handles that are Types
    if (isType(h)) {
        throw RuntimeException(TRACE_INFO, "Invalid fake handle %u: it's actually a type", h);
    }
    vhmap_t::const_iterator i = vhmap.find(h);
    if (i != vhmap.end()) {
        // check that real Handle is still valid
        if (TLB::isValidHandle(i->second.first)) {
            // return existing fake handle
            return i->second;
        } else {
            //! @todo remove fake Handle
            throw RuntimeException(TRACE_INFO, "fake handle %u points to "
                    "a now invalid handle", h);
        }
    } else {
        throw RuntimeException(TRACE_INFO, "Invalid fake handle %u", h);
    }

}

pHandle AtomSpaceWrapper::realToFakeHandle(Handle h, VersionHandle vh)
{
    // check if already exists
    vhmap_reverse_t::const_iterator i = vhmap_reverse.find(vhpair(h,vh));
    if (i != vhmap_reverse.end()) {
        // return existing fake handle
        return i->second;
    } else {
        // add to vhmap
        pHandle fakeHandle = (pHandle) vhmap.size() + mapOffset;
        if (fakeHandle < mapOffset) {
            // Error: too many version to handle mappings!
            Logger().error("too many version-to-handle mappings!");
            exit(-1);
        }
        vhmap.insert( vhmap_pair_t(fakeHandle, vhpair(h,vh)) );
        vhmap_reverse.insert( vhmap_reverse_pair_t(vhpair(h,vh), fakeHandle) );
        return fakeHandle;
    }

}

std::vector< pHandle > AtomSpaceWrapper::realToFakeHandle(const Handle h) {
    std::vector< pHandle > result;
    result.push_back(realToFakeHandle(h, NULL_VERSION_HANDLE));
    if (AS_PTR->getTV(h).getType() == COMPOSITE_TRUTH_VALUE) {
        const CompositeTruthValue ctv = dynamic_cast<const CompositeTruthValue&> (AS_PTR->getTV(h));
        for (int i = 0; i < ctv.getNumberOfVersionedTVs(); i++) { 
            VersionHandle vh = ctv.getVersionHandle(i);
            if (dummyContexts.find(vh) != dummyContexts.end()) {
                // if dummyContext contains a VersionHandle for h
                result.push_back( realToFakeHandle(h, vh));
            }
        }
    }
    return result;
}

std::vector< pHandle > AtomSpaceWrapper::realToFakeHandles(std::vector< Handle > hs, bool expand) {
    std::vector<pHandle> result;
    foreach (Handle h, hs) {
        if (expand) 
            mergeCopy(result,realToFakeHandle(h));
        else
            result.push_back(realToFakeHandle(h,NULL_VERSION_HANDLE));
    }
    return result;
}

pHandleSeq AtomSpaceWrapper::realToFakeHandles(const Handle h, const Handle c)
{
    // c is a context whose outgoing set of contexts matches contexts of each
    // handle in outgoing of real handle h.
    HandleSeq contexts = AS_PTR->getOutgoing(c);
    HandleSeq outgoing = AS_PTR->getOutgoing(h);
    pHandleSeq results;

    // Check that all contexts match... they should
    bool match = true;
    for (unsigned int i=0; match && i < outgoing.size(); i++) {
        // +1 because first context is for distinguishing dual links using the
        // same destination contexts.
        if (AS_PTR->getName(contexts[i+1]) == rootContext)
            contexts[i+1] = Handle::UNDEFINED;
        VersionHandle vh(CONTEXTUAL, contexts[i+1]);
        if (AS_PTR->getTV(outgoing[i],vh).isNullTv()) {
            match = false;
        } else {
            results.push_back(realToFakeHandle(outgoing[i], vh));
        }
    }
    if (match)
        return results;
    else
        throw RuntimeException(TRACE_INFO, "getOutgoing: link context is bad");
        //return realToFakeHandles(s1, NULL_VERSION_HANDLE);
}

const TruthValue& AtomSpaceWrapper::getTV(pHandle h)
{
    AtomSpace* a = AS_PTR;
    if (h != PHANDLE_UNDEFINED) {
        vhpair r = fakeToRealHandle(h);
        return a->getTV(r.first,r.second);
    } else {
        return TruthValue::TRIVIAL_TV();
    }
}

shared_ptr<set<pHandle> > AtomSpaceWrapper::getHandleSet(Type T, const string& name, bool subclass) 
{
    HandleEntry* result = 
        (name.empty()
            ? AS_PTR->getAtomTable().getHandleSet((Type) T, subclass)
            : AS_PTR->getAtomTable().getHandleSet(name.c_str(), (Type) T, subclass));
    shared_ptr<set<Handle> > ret(new set<Handle>);

    HandleEntry2HandleSet(*result, *ret);
    delete result;

    shared_ptr<set<pHandle> > retFake(new set<pHandle>);
    foreach (Handle h, *ret) {
        foreach(pHandle h2, realToFakeHandle(h)) {
            (*retFake).insert(h2); 
        }
    }
    return retFake;
}

pHandle AtomSpaceWrapper::getHandle(Type t,const string& name) 
{
    return realToFakeHandle(AS_PTR->getAtomTable().getHandle(name.c_str(), (Type)t),
            NULL_VERSION_HANDLE);
}

bool AtomSpaceWrapper::equal(const HandleSeq& lhs, const HandleSeq& rhs)
{
    size_t lhs_arity = lhs.size();
    if (lhs_arity != rhs.size())
        return false;
        
    for (unsigned int i = 0; i < lhs_arity; i++)
        if (lhs[i] != rhs[i])
            return false;
    return true;            
}

pHandle AtomSpaceWrapper::getHandle(Type t,const pHandleSeq& outgoing)
{
    HandleSeq outgoingReal;
    std::vector<VersionHandle> vhs;
    foreach (pHandle h, outgoing) {
        vhpair v = fakeToRealHandle(h);
        outgoingReal.push_back(v.first);
        vhs.push_back(v.second);
    }
    // get real handle, and then check whether link has appropriate context
    // compared to outgoing set, otherwise return NULL_VERSION_HANDLE link
    // in order to find appropriate context you also need to find the common
    // context of the outgoing set. either that or a context that inherits from
    // all the contexts of outgoing set.
    Handle real = AS_PTR->getHandle(t,outgoingReal);
    // Find a a versionhandle with a context that has the same order of contexts as
    // vhs, otherwise return default
    const TruthValue& tv = AS_PTR->getTV(real);
    if (tv.getType() == COMPOSITE_TRUTH_VALUE) {
        const CompositeTruthValue ctv = dynamic_cast<const CompositeTruthValue&> (tv);
        for (int i = 0; i < ctv.getNumberOfVersionedTVs(); i++) { 
            VersionHandle vh = ctv.getVersionHandle(i);
            HandleSeq hs = AS_PTR->getOutgoing(vh.substantive);
            bool matches = true;
            assert(hs.size() == (vhs.size()+1));
            for (unsigned int j = 0; j < vhs.size()-1; j++) {
                if (hs[j+1] != vhs[j].substantive) {
                    matches = false;
                    break;
                }
            }
            if (matches)
                return realToFakeHandle(real,vh);
        }
    }
    return realToFakeHandle(real,NULL_VERSION_HANDLE);
}

void AtomSpaceWrapper::reset()
{
    dummyContexts.clear();
    vhmap.clear();
    vhmap_reverse.clear();
//    ::haxx::childOf.clear();
    ::haxx::variableShadowMap. clear();
    AS_PTR->clear();
    AS_PTR->addNode(CONCEPT_NODE, rootContext);
}

//#endif

extern int atom_alloc_count;
//void initArbitraryAtoms();
//void printUtable();
//void SetTheoremTest();
//void existence2universality();
static Handle U;

bool AtomSpaceWrapper::loadAxioms(const string& path)
{
    // TODO: check exists works on WIN32
    string fname(path);
    string fname2("../tests/reasoning/" + path);
//    string fname2(PLN_TEST_DIR + path);
//    string fname2("tests/reasoning/" + path);
    if (!exists(fname.c_str())) {
        printf("File %s doesn't exist.\n", fname.c_str());
        fname = fname2;
    }
    if (!exists(fname.c_str())) {
        printf("File %s doesn't exist.\n", fname.c_str());
        return false;
    }
    
    try {
        printf("Loading axioms from: %s \n", fname.c_str());        
//        U = LoadXMLFile(this, fname);
        cprintf(5, "thms clear...");
    	CrispTheoremRule::thms.clear();
    	
        // Use the XML reader only if XML is available.
#if HAVE_EXPAT
        std::vector<XMLBufferReader*> readers(1, new FileXMLBufferReader(fname.c_str()));
        NMXmlParser::loadXML(readers, AS_PTR);        
        delete readers[0];
#else
        LOG(0, "Thive version of opencog has no XML support"); 
        return false;
#endif /* HAVE_EXPAT */

        // re-generate CrispTheoremRule::thms
        makeTheorems();
        
        loadedFiles.insert(fname);
    } catch(string s) { 
        LOG(0, s); 
        return false; 
    } catch(...) { 
        LOG(0, "UNKNOWN EXCEPTION IN LOADAXIOMS!!!"); 
        return false; 
    }

    return true;
}

bool AtomSpaceWrapper::loadOther(const string& path, bool ReplaceOld)
{
    string buf;
    LoadTextFile(path, buf);

    vector<string> lines = StringTokenizer(buf, "\n\r").WithoutEmpty();

//  SetUniverseSize(CONST_SIZE, lines.size());

    for (uint i = 0; i < lines.size(); i++)
    {
        vector<string> mainelems = StringTokenizer(lines[i], "(").WithoutEmpty();
        if (mainelems.size()<2)
            continue;

        float percentage = 0.0f;

        percentage = atof(StringTokenizer(mainelems[1], "%")[0].c_str());

        vector<string> elems = StringTokenizer(mainelems[0], "\t ").WithoutEmpty();


        SimpleTruthValue tv(percentage/100.0f, 1);

        if (elems.size() == 1)
            addNode(CONCEPT_NODE, elems[0],
                tv,
                false,
                false);
        else if (!elems.empty())
        {
            pHandleSeq hs;

            for (unsigned int j = 0; j < elems.size(); j++)
                if (!elems[j].empty())
                    hs.push_back(getHandle(CONCEPT_NODE, elems[j]));
               
            assert (hs.size()>1);

            addLink(AND_LINK, hs, tv, false, false);
        }
    }

    loadedFiles.insert(path);

    return true;
}

/*vector<atom> AtomSpaceWrapper::LoadAnds(const string& path)
{
    string buf;
    LoadTextFile(path, buf);

    vector<atom> ret;

    puts("Adding the ANDs");

    vector<string> lines = StringTokenizer(buf, "\n\r").WithoutEmpty();
    for (int i = 0; i < lines.size(); i++)
    {
        vector<string> elems = StringTokenizer(lines[i], "\t ").WithoutEmpty();

        if(elems.size() ==1)
        {
            puts("Warning! 1-node link!");
            puts(elems[0].c_str());
        }

        if (elems.size()>=2)
        {
            vector<atom> hs;

            for (int j = 0; j < elems.size(); j++)
                if (!elems[j].empty())
                    hs.push_back(atom(CONCEPT_NODE, elems[j]));

            if (hs.size()==1)
            {
                puts("Warning! 1-node link!");
                printAtomTree(hs[0]);
            }
            else
                ret.push_back(atom(AND_LINK, hs));
        }
        printf("%d\n", i);
    }

    puts("ANDs ok");

    return ret;
}

*/

///////////

#define P_DEBUG 0

int AtomSpaceWrapper::getFirstIndexOfType(const pHandleSeq hs, const Type T) const
{
    AtomSpace *a = AS_PTR;
    for (unsigned int i = 0; i < hs.size(); i++)
        if (a->getType(fakeToRealHandle(hs[i]).first) == T)
            return i;
    return -1;
}

#if 0
void remove_redundant(HandleSeq& hs)
{
char b[200];
sprintf(b, "H size before removal: %d.", hs.size());
LOG(5, b);
    
    for (vector<Handle>::iterator ii = hs.begin(); ii != hs.end(); ii++)
    {
        atom atom_ii(*ii);

        for (vector<Handle>::iterator jj = ii; jj != hs.end();)
            if (++jj != hs.end())
                if (atom(*jj) == atom_ii)
/*              if (TheHandleWrapperFactory.New(*jj)->equalOLD(
                            *TheHandleWrapperFactory.New(*ii)
                  ))*/
                {
#ifndef WIN32
    THE FOLLOWING MAY NOT WORK OUTSIDE WIN32:
#endif

sprintf(b, "Removing %s (%d).", a->getName(*ii).c_str(), nm->getType(*ii));
LOG(5, b);

                    jj = hs.erase(jj);
                    jj--;
                }
    }

sprintf(b, "H size AFTER removal: %d.", hs.size());
LOG(5, b);

/*  for (i = 0; i < hs.size(); i++)
        hws.push_back(TheHandleWrapperFactory.New(hs[i]));

    for (vector<HandleWrapper*>::iterator ii = hws.begin(); ii != hws.end(); ii++)
        for (vector<HandleWrapper*>::iterator jj = ii; jj != hws.end();)
            if (++jj != hws.end())
                if ((*jj)->equalOLD(**ii))
                {
#ifndef WIN32
    THE FOLLOWING MAY NOT WORK OUTSIDE WIN32:
#endif
                    jj = hws.erase(jj);
                    jj--;
                }*/
}
#endif

bool AtomSpaceWrapper::binaryTrue(pHandle h)
{
    const TruthValue& tv = getTV(h);//fakeToRealHandle(h).first);

    return (tv.getMean() > PLN_TRUE_MEAN);
}

bool AtomSpaceWrapper::symmetricLink(Type T)
{
    /// Only used by obsolete code in NormalizingATW::addLink
    // TODO: either remove or replace with a symmetric link super class...
    // or make sure it's equivalent.
    return inheritsType(T, AND_LINK) || inheritsType(T, LIST_LINK)
            || inheritsType(T, OR_LINK);
}

bool AtomSpaceWrapper::isEmptyLink(pHandle h)
{
    return !inheritsType(getType(h), NODE)
            && getArity(h) == 0;
}

bool AtomSpaceWrapper::hasFalsum(pHandleSeq hs)
{
    for (pHandleSeq::const_iterator ii = hs.begin(); ii != hs.end(); ii++)
    {
        const pHandle key = *ii;

        if (inheritsType(getType(key), FALSE_LINK) ) //Explicit falsum
            return true;

        for (pHandleSeq::const_iterator jj = hs.begin(); jj != hs.end();jj++) {
            if (jj != ii) {
                if (inheritsType(getType(*jj), NOT_LINK) ) //Contradiction
                {
                    pHandle notter = getOutgoing(*jj)[0];
                    if (notter == key)
                        return true;
                }
            }
        }
    }
    return false;
}

bool AtomSpaceWrapper::containsNegation(pHandle ANDlink, pHandle h)
{
    pHandleSeq hs = getOutgoing(ANDlink);
    hs.push_back(h);
    return hasFalsum(hs);
}

pHandle AtomSpaceWrapper::freshened(pHandle h, bool managed)
{
    Type T = getType(h);
    pHandleSeq hs = getOutgoing(h);
    string name = getName(h);
    const TruthValue& tv = getTV(h);

    if (inheritsType(T, NODE))
        return addNode(T, name, tv, true,managed);
    else
    {
        for (unsigned int i = 0; i < hs.size(); i++)
            hs[i] = freshened(hs[i], managed);

        return addLink(T, hs, tv, true, managed);
    }
}

// change to
// Handle AtomSpaceWrapper::addAtom(vtree& a, const TruthValue& tvn, bool fresh, bool managed, Handle associateWith)
pHandle AtomSpaceWrapper::addAtom(vtree& a, const TruthValue& tvn, bool fresh, bool managed)
{
    return addAtom(a,a.begin(),tvn,fresh,managed);
}

pHandle AtomSpaceWrapper::addAtom(vtree& a, vtree::iterator it, const TruthValue& tvn, bool fresh, bool managed)
{
    AtomSpace *as = AS_PTR;
    cprintf(3,"Handle AtomSpaceWrapper::addAtom...");
    rawPrint(a,it,3);
    
    pHandleSeq handles;
    pHandle head_type = boost::get<pHandle>(*it);
    //assert(haxx::AllowFW_VARIABLENODESinCore || head_type != (Handle)FW_VARIABLE_NODE);
    //vector<Handle> contexts;
    
    if (!isType(head_type))
    {
        LOG(1, "Warning! Trying to add a real atom with addAtom(vtree& a), returning type!\n");
        return head_type;
    }

    for (vtree::sibling_iterator i = a.begin(it); i!=a.end(it); i++)
    {
        pHandle *h_ptr = boost::get<pHandle>(&*i);

        pHandle added = (h_ptr != NULL && !isType(*h_ptr)) ?
            (*h_ptr) : addAtom(a, i, TruthValue::TRIVIAL_TV(), false, managed);
        handles.push_back(added);
    }

/*  /// We cannot add non-existent nodes this way!
    assert (!inheritsType(head_type, NODE));*/
    
    /// Comment out because addLink should handle the contexts
    //if (contexts.size() > 1) {
    //    LOG(1, "Contexts are different, implement me!\n");
    //}
    // Provide new context to addLink function
    return addLink(head_type, handles, tvn, fresh, managed);
}

pHandle AtomSpaceWrapper::directAddLink(Type T, const pHandleSeq& hs, const TruthValue& tvn,
        bool fresh,bool managed)
{
    AtomSpace *a = AS_PTR;
    if (tvn.isNullTv())
    {
        LOG(0, "I don't like FactoryTruthValues, so passing "
                "NULL as TruthValue throws exception in AtomSpaceWrapper.cc.");
        throw RuntimeException(TRACE_INFO, "NULL TV passed to directAddLink");
    }

    LOG(3, "Directly adding...");

    pHandle ret;
    uint arity = hs.size();
    
    if (T == INHERITANCE_LINK && arity==2) {
/*        // childOf keeps a record of inheritance
        // It was tempting to change childOf to use real handles so that maybe
        // fewer of these pairings need to be stored. But this won't work
        // because each Versioned link is specific.
        haxx::childOf.insert(hpair(hs[1], hs[0]));*/
    }

    // TODO should this still be here also?
    // if we are archiving theorems, and trying to add a implication link
    // composed of AND as a source, and the TruthValue is essentially true
    if (haxx::ArchiveTheorems &&
        T == IMPLICATION_LINK && getType(hs[0]) == AND_LINK &&
        tvn.getConfidence() > PLN_TRUE_MEAN) {
            pHandleSeq args = getOutgoing(hs[0]);
            cprintf(-3,"THM for:");

            vtree thm_target(make_vtree(hs[1]));

            rawPrint(thm_target, thm_target.begin(), 3);
            LOG(0,"Takes:");
            
            foreach(pHandle arg, args) {
                vtree arg_tree(make_vtree(arg));
                rawPrint(arg_tree, arg_tree.begin(), 0);
                CrispTheoremRule::thms[thm_target].push_back(arg_tree);
            }
            
            ret = addLinkDC( FALSE_LINK, hs, tvn, fresh, managed);
    } else {
        ret = addLinkDC( T, hs,  tvn, fresh, managed);
    }

    if (inheritsType(T, LINK) && !arity && T != FORALL_LINK) {
        // Link with no connections?
        printTree(ret,0,1);
        cprintf(1,"inheritsType(T, LINK) && !arity\n");
    }   
    
    if (!haxx::AllowFW_VARIABLENODESinCore) {
        foreach(pHandle ch, hs) {

            assert(!isType(ch));
            if (getType(ch) == FW_VARIABLE_NODE) {
                printTree(ret,0,-10);
                cprintf(-10,"ATW: getType(ch) == FW_VARIABLE_NODE!");
                getc(stdin);
            }
            //throw string("a->getType(ch) == FW_VARIABLE_NODE") + i2str(ret);
            //assert(a->getType(ch) != FW_VARIABLE_NODE);
        }
    }
        
#if USE_MIND_SHADOW
    haxx::mindShadow.push_back(ret);
    haxx::mindShadowMap[T].push_back(ret);
#endif
LOG(3, "Add ok.");

/*  if (!tvn.isNullTv())
        if (Abs(a->getTV(ret)->getMean() - tvn.getMean()) > 0.0001)
        {
            printf("ATW: %s / %s\n", a->getTV(ret)->toString().c_str(),
                tvn.toString().c_str());
        }*/
    
//  assert(Abs(a->getTV(ret)->getMean() - tvn.getMean()) < 0.0001);

    return ret;
}

void AtomSpaceWrapper::makeTheorems() {
    // if we are archiving theorems
    if (haxx::ArchiveTheorems) {    
        // for each implication link composed of AND as a source, and whose
        // TruthValue is essentially true
        
        // TODO this could use the index that involves complex atom structure predicates
        Btr<set<pHandle> > links = getHandleSet(IMPLICATION_LINK, "");
        foreach(pHandle h, *links)
        {
            const pHandleSeq hs = getOutgoing(h);
            const TruthValue& tvn = getTV(h);
            
            if(getType(hs[0]) == AND_LINK &&
            tvn.getConfidence() > PLN_TRUE_MEAN) {
                pHandleSeq args = getOutgoing(hs[0]);
                cprintf(-3,"THM for:");

                vtree thm_target(make_vtree(hs[1]));

                rawPrint(thm_target, thm_target.begin(), 3);
                LOG(0,"Takes:");
                
                foreach(pHandle arg, args) {
                    vtree arg_tree(make_vtree(arg));
                    rawPrint(arg_tree, arg_tree.begin(), 0);
                    CrispTheoremRule::thms[thm_target].push_back(arg_tree);
                }
                // TODO doesn't convert the ImplicationLink into a FalseLink
                // (is that necessary / appropriate?)
            }
        }
    }
}

//int AtomSpaceWrapper::implicationConstruction()
//{
//  return 0;
/*  Btr<set<Handle> > links = getHandleSet(AND_LINK,"");
    foreach(Handle h, *links)
    {
    }*/
//}


pHandle AtomSpaceWrapper::addLinkDC(Type t, const pHandleSeq& hs, const TruthValue& tvn,
        bool fresh, bool managed)
{
    AtomSpace *a = AS_PTR;
    pHandle ret;
    HandleSeq hsReal;
    HandleSeq contexts;
    // Convert outgoing links to real Handles
    foreach(pHandle h, hs) {
        vhpair v = fakeToRealHandle(h);
        hsReal.push_back(v.first);
        if (v.second.substantive == Handle::UNDEFINED) {
            contexts.push_back(AS_PTR->getHandle(CONCEPT_NODE, rootContext));
        } else {
            contexts.push_back(v.second.substantive);
        }
    }
    
    // Construct a Link then use addAtomDC
    Link l(t,hsReal,tvn);
    ret = addAtomDC(l, fresh, managed, contexts);
    return ret;
}

pHandle AtomSpaceWrapper::addNodeDC(Type t, const string& name, const TruthValue& tvn,
        bool fresh, bool managed)
{
    AtomSpace *a = AS_PTR;
    // Construct a Node then use addAtomDC
    Node n(t, name, tvn);
    return addAtomDC(n, fresh, managed);
}

pHandle AtomSpaceWrapper::addAtomDC(Atom &atom, bool fresh, bool managed, HandleSeq contexts)
{
    AtomSpace *a = AS_PTR;
    Handle result;
    pHandle fakeHandle;
    // check if fresh true
    if (fresh) {
        // See if atom exists already
        //const HandleSeq outgoing;
        Node *nnn = dynamic_cast<Node *>((Atom *) & atom);
        if (nnn) {
            const Node& node = (const Node&) atom;
            result = a->getHandle(node.getType(), node.getName());
            if (TLB::isInvalidHandle(result)) {
                // if the atom doesn't exist already, then just add normally
                return realToFakeHandle(a->addNode(node.getType(),
                        node.getName(), node.getTruthValue()),
                        NULL_VERSION_HANDLE);
            }
        } else {
            const Link& link = (const Link&) atom;
            //for (int i = 0; i < link.getArity(); i++) {
            //    outgoing.push_back(TLB::getHandle(link.getOutgoingAtom(i)));
            //}
            //outgoing = link.getOutgoingSet();
            result = a->getHandle(link.getType(), link.getOutgoingSet());
            if (TLB::isInvalidHandle(result)) {
                // if the atom doesn't exist already, then add normally
                bool allNull = true;
                // if all null context
                for (HandleSeq::iterator i = contexts.begin();
                        i != contexts.end();
                        i++) {
                    if (AS_PTR->getName(*i) != rootContext) {
                        allNull = false;

                    }
                }
                result = a->addLink(link.getType(), link.getOutgoingSet(),
                        TruthValue::TRIVIAL_TV());
                fakeHandle = realToFakeHandle(result, NULL_VERSION_HANDLE);
                if (allNull) {
                    a->setTV(result, atom.getTruthValue(), NULL_VERSION_HANDLE);
                    return fakeHandle;
                } else {
                    printf("Not all contexts of new link are null! Needs to be "
                            "implemented in AtomSpaceWrapper...");
                    char c;
                    cin >> c;
                    /// @todo create link context if contexts are not all null
                    /// but possibly not needed...
                    assert(0);
                    // create link context
                    //link.getTruthValue();
                }
            }
        }
        // if it does exist then
        // result contains a handle to existing atom
        VersionHandle vh;

        // build context link's outgoingset if necessary
       /* // if this is a node with nothing in contexts
        if (contexts.size() == 0) {
            assert(nnn);
            // @todo
            // find and create free context:
            // find last context in composite truth value, use as dest of new
            // context
            contexts.push_back(AS_PTR->getHandle(CONCEPT_NODE, rootContext));

        } */
        Handle contextLink = getNewContextLink(result,contexts);
        vh = VersionHandle(CONTEXTUAL,contextLink);
        // add version handle to dummyContexts
        dummyContexts.insert(vh);
        // vh is now a version handle for a free context
        // for which we can set a truth value
        a->setTV(result, atom.getTruthValue(), vh);

        // Link <handle,vh> to a long int
        fakeHandle = realToFakeHandle(result,vh);
    } else {
        // no fresh:
        VersionHandle vh = NULL_VERSION_HANDLE;
        if (contexts.size() != 0) {
            // if the atom doesn't exist already, then add normally
            bool allNull = true;
            // if all null context
            for (HandleSeq::iterator i = contexts.begin();
                    i != contexts.end();
                    i++) {
                if (AS_PTR->getName(*i) != rootContext) {
                    allNull = false;

                }
            }
            // Get the existing context link if not all contexts are NULL 
            if (!allNull) {
                contexts.insert(contexts.begin(),AS_PTR->getHandle(CONCEPT_NODE, rootContext));
                Handle existingContext = AS_PTR->getHandle(ORDERED_LINK,contexts);
                if (TLB::isInvalidHandle(existingContext)) {
                    existingContext = a->addLink(ORDERED_LINK,contexts);
                    vh = VersionHandle(CONTEXTUAL,existingContext);
                    dummyContexts.insert(vh);
                } else {
                    vh = VersionHandle(CONTEXTUAL,existingContext);
                }
            }
        }
        // add it and let AtomSpace deal with merging it
        result = a->addRealAtom(atom);
        if (vh != NULL_VERSION_HANDLE) {
            // if it's not for the root context, we still have to
            // specify the truth value for that VersionHandle
            AS_PTR->setTV(result, atom.getTruthValue(), vh);
        }
        fakeHandle = realToFakeHandle(result, vh);
        
    }
    return fakeHandle;

}

Handle AtomSpaceWrapper::getNewContextLink(Handle h, HandleSeq contexts) {
    // insert root as beginning
    contexts.insert(contexts.begin(),AS_PTR->getHandle(CONCEPT_NODE, rootContext));

    // check if root context link exists
    Handle existingLink = AS_PTR->getHandle(ORDERED_LINK,contexts);
    if(TLB::isInvalidHandle(existingLink)) {
        return AS_PTR->addLink(ORDERED_LINK,contexts);
    } else {
        // if it does exist, check if it's in the contexts of the
        // given atom
        if (AS_PTR->getTV(h).getType() == COMPOSITE_TRUTH_VALUE) {
            const CompositeTruthValue ctv = dynamic_cast<const CompositeTruthValue&> (AS_PTR->getTV(h));
            bool found = false;
            do {
                found = false;
                for (int i = 0; i < ctv.getNumberOfVersionedTVs(); i++) { 
                    VersionHandle vh = ctv.getVersionHandle(i);
                    if (AS_PTR->getOutgoing(vh.substantive,0) == existingLink) {
                        existingLink = vh.substantive;
                        found = true;
                    }
                }
            } while (found);
            // existingLink is now the furthest from rootContext
            contexts[0] = existingLink;
        }
        return AS_PTR->addLink(ORDERED_LINK,contexts);
    }
}

bool AtomSpaceWrapper::removeAtom(pHandle h)
{
    AtomSpace *a = AS_PTR;
    // remove atom
    vhpair v = fakeToRealHandle(h);
    if (v.second == NULL_VERSION_HANDLE) {
        a->removeAtom(v.first);
    } else {
        const TruthValue& currentTv = a->getTV(v.first);
        CompositeTruthValue ctv = CompositeTruthValue(
                (const CompositeTruthValue&) currentTv);
        ctv.removeVersionedTV(v.second);
    }
    // remove fake handle
    vhmap_t::iterator j, i;
    vhmap_reverse_t::iterator ir;
    i = vhmap.find(h);
    if (i != vhmap.end()) {
        vhmap.erase(i);
        ir = vhmap_reverse.find(i->second);
        if (ir != vhmap_reverse.end()) {
            vhmap_reverse.erase(ir);
        }
    }
    // check map to see whether real handles are still valid (because removing
    // an atom might remove links connecting to it, and removing a
    // NULL_VERSION_HANDLE atom will remove all the versions of an atom)
    for ( i = vhmap.begin(); i != vhmap.end(); i++ ) {
        if (TLB::getAtom(i->second.first)) {
            j = i;
            i--;
            vhmap.erase(j);
            ir = vhmap_reverse.find(j->second);
            if ( ir != vhmap_reverse.end() ) vhmap_reverse.erase(ir);
        }
    }
    // @todo - also remove freed dummy contexts
    return true;
}


pHandle AtomSpaceWrapper::getRandomHandle(Type T)
{
    AtomSpace *a = AS_PTR;
    vector<Handle> handles=a->filter_type(T);

    if (handles.size()==0)
        return PHANDLE_UNDEFINED;

    return realToFakeHandle(handles[rand()%handles.size()], NULL_VERSION_HANDLE);
}

pHandleSeq AtomSpaceWrapper::getImportantHandles(int number)
{
    // TODO: check all VersionHandles for the highest importance
    AtomSpace *a = AS_PTR;
    std::vector<Handle> hs;

    a->getHandleSetInAttentionalFocus(back_inserter(hs), ATOM, true);
    int toRemove = hs.size() - number;
    sort(hs.begin(), hs.end(), compareSTI());
    if (toRemove > 0) {
        while (toRemove > 0) {
            hs.pop_back();
            toRemove--;
        }
    }
    return realToFakeHandles(hs);

}

#if 0
unsigned int USize(const set<Handle>& triples, const set<Handle>& doubles,
                   const set<Handle>& singles)
{
    map<Handle, tuple3<Handle> > triple_contents;
    map<Handle, tuple2<Handle> > double_contents;
    
    int i=0;
    float total_n = 0.0f, nA, nB, nC, nAB, nBC, nAC, nABC;

    for (set<Handle>::const_iterator t = triples.begin(); t != triples.end(); t++)
    {
        nABC = TheNM.a->getCount(*t);

        vector<Handle> tc = a->getOutgoing(*t);
        assert(tc.size()==3);

        tuple2<Handle> ab, ac, bc;

        ab.t1 = tc[0];
        ab.t2 = tc[1];
        ac.t1 = tc[0];
        ac.t2 = tc[2];
        bc.t1 = tc[1];
        bc.t2 = tc[2];

        for (set<Handle>::const_iterator d = doubles.begin(); d != doubles.end(); d++)
        {
            vector<Handle> dc = a->getOutgoing(*d);
            assert(dc.size()==2);

            if (dc[0] == ab.t1 && dc[1] == ab.t2)
                nAB = TheNM.a->getCount(*d);
            else if (dc[0] == ac.t1 && dc[1] == ac.t2)
                nAC = TheNM.a->getCount(*d);
            else if (dc[1] == bc.t1 && dc[2] == bc.t2)
                nBC = TheNM.a->getCount(*d);
        }

        nA = TheNM.a->getCount(ab.t1);
        nB = TheNM.a->getCount(ab.t2);
        nC = TheNM.a->getCount(ac.t2);

        total_n += nB + (nA - nAB)*(nC - nBC) / (nAC - nABC);
    }

    return total_n / triples.size();
}
#endif


void AtomSpaceWrapper::DumpCoreLinks(int logLevel)
{
/*#ifdef USE_PSEUDOCORE
      PseudoCore::LinkMap LM = ((PseudoCore*)nm)->getLinkMap();

      for (PseudoCore::LinkMap::iterator i = LM.begin(); i != LM.end(); i++)
          printTree((Handle)i->first,0,logLevel);
#else
puts("Dump disabled");
#endif*/
  pHandleSeq LM=filter_type(LINK);
  for (pHandleSeq::iterator i = LM.begin(); i != LM.end(); i++)
    printTree(*i,0,logLevel);
}

void AtomSpaceWrapper::DumpCoreNodes(int logLevel)
{
/*#ifdef USE_PSEUDOCORE
      PseudoCore::NodeMap LM = ((PseudoCore*)nm)->getNodeMap();

      for (PseudoCore::NodeMap::iterator i = LM.begin(); i != LM.end(); i++)
          printTree((Handle)i->first,0,logLevel);
#else
puts("Dump disabled");
#endif*/
  pHandleSeq LM=filter_type(NODE);
  for (pHandleSeq::iterator i = LM.begin(); i != LM.end(); i++)
    printTree(*i,0,logLevel);
}

void AtomSpaceWrapper::DumpCore(Type T)
{
    shared_ptr<set<pHandle> > fa = getHandleSet(T,"");
    //for_each<TableGather::iterator, handle_print<0> >(fa.begin(), fa.end(), handle_print<0>());
    for_each(fa->begin(), fa->end(), handle_print<0>());
}

Handle singular(HandleSeq hs) {
    assert(hs.size()<=1);
    return !hs.empty() ? hs[0] : Handle::UNDEFINED;
}

/*Handle list_atom(HandleSeq hs) {  
    return addLink(LIST_LINK, hs, TruthValue::TRUE_TV(),false);
}*/

bool AtomSpaceWrapper::equal(Handle A, Handle B)
{
    AtomSpace *a = AS_PTR;
    if (a->getType(A) != a->getType(B))
        return false;

    vector<Handle> hsA = a->getOutgoing(A);
    vector<Handle> hsB = a->getOutgoing(B);

    const size_t Asize = hsA.size();

    if (Asize != hsB.size())
        return false;

    for (unsigned int i = 0; i < Asize; i++)
        if (!equal(hsA[i], hsB[i]))
            return false;
    
    return true;
}

pHandle AtomSpaceWrapper::OR2ANDLink(pHandle& andL)
{
    return AND2ORLink(andL, OR_LINK, AND_LINK);
}

pHandle AtomSpaceWrapper::AND2ORLink(pHandle& andL)
{
    return AND2ORLink(andL, AND_LINK, OR_LINK);
}

pHandle AtomSpaceWrapper::invert(pHandle h)
{
    pHandleSeq hs;
    hs.push_back(h);
    return addLink(NOT_LINK, hs, TruthValue::TRUE_TV(), true);
}

pHandle AtomSpaceWrapper::AND2ORLink(pHandle& andL, Type _ANDLinkType, Type _ORLinkType)
{
    assert(getType(andL) == _ANDLinkType);

    pHandleSeq ORtarget;
    const pHandleSeq _ANDtargets = getOutgoing(andL);

    for (pHandleSeq::const_iterator i = _ANDtargets.begin();
            i != _ANDtargets.end(); i++) {
        ORtarget.push_back(invert(*i));
    }

    const TruthValue& outerTV = getTV(andL);

    pHandleSeq NOTarg;
    pHandle newORAND = addLink(_ORLinkType, ORtarget,
                outerTV,
                true);

    NOTarg.push_back(newORAND);

puts("---------");
printTree(newORAND,0,0);

    return addLink(NOT_LINK, NOTarg,
                TruthValue::TRUE_TV(),
                true);
}

pair<pHandle, pHandle> AtomSpaceWrapper::Equi2ImpLink(pHandle& exL)
{
    printf("((%d))\n", getType(exL));
    printTree(exL,0,0);

    assert(getType(exL) == EXTENSIONAL_EQUIVALENCE_LINK);

    pHandleSeq ImpTarget1, ImpTarget2;

    const pHandleSeq EquiTarget = getOutgoing(exL);

    for (pHandleSeq::const_iterator i = EquiTarget.begin(); i != EquiTarget.end(); i++)
        ImpTarget1.push_back((*i));

    assert(ImpTarget1.size()==2);

    for (pHandleSeq::const_reverse_iterator j = EquiTarget.rbegin(); j != EquiTarget.rend(); j++)
        ImpTarget2.push_back((*j));

    assert(ImpTarget2.size()==2);

    const TruthValue& outerTV = getTV(exL);

    pair<pHandle, pHandle> ret;

    ret.first = addLink(IMPLICATION_LINK, ImpTarget1,
                outerTV,
                true);

    ret.second = addLink(IMPLICATION_LINK, ImpTarget2,
                outerTV,
                true);

    return ret;
}

bool AtomSpaceWrapper::isType(const pHandle h) const
{
    return h < mapOffset && h != PHANDLE_UNDEFINED;
}

const TimeServer& AtomSpaceWrapper::getTimeServer() const
{
    return AS_PTR->getTimeServer();
}

int AtomSpaceWrapper::getArity(pHandle h) const
{
    // get neighbours
    vhpair v = fakeToRealHandle(h);
    // check neighbours have TV with same VersionHandle
    // HandleSeq hs = AS_PTR->getOutgoing(v.first);
    int arity = AS_PTR->getArity(v.first);
    /*foreach (Handle h, hs) {
        if (!AS_PTR->getTV(v.first,v.second).isNullTv()) {
            arity += 1;
        }
    }*/
    return arity;

}

pHandleSeq AtomSpaceWrapper::filter_type(Type t) 
{
    HandleSeq s;
    s = AS_PTR->filter_type(t);
    return realToFakeHandles(s, true);

}

Type AtomSpaceWrapper::getType(const pHandle h) const
{
    return AS_PTR->getType(fakeToRealHandle(h).first);
}

std::string AtomSpaceWrapper::getName(const pHandle h) const
{
    return AS_PTR->getName(fakeToRealHandle(h).first);
}

Type AtomSpaceWrapper::getTypeV(const tree<Vertex>& _target) const
{
    // fprintf(stdout,"Atom space address: %p\n", this);
    // fflush(stdout);
    return getType(boost::get<pHandle>(*_target.begin()));
}

/*
Handle AtomSpaceWrapper::Exist2ForAllLink(Handle& exL)
{
    assert(a->getType(exL) == EXIST_LINK);

    HandleSeq ForAllTarget;

    const vector<Handle> ExistTarget = a->getOutgoing(exL);

    for (vector<Handle>::const_iterator i = ExistTarget.begin(); i != ExistTarget.end(); i++)
    {
        ForAllTarget.push_back(invert(*i));
    }

    const TruthValue& outerTV = getTV(exL);

    HandleSeq NOTarg;
    NOTarg.push_back(addLink(FORALL_LINK, ForAllTarget,
                outerTV,
                true));

    return addLink(NOT_LINK, NOTarg,
                TruthValue::TRUE_TV(),
                true);
}
*/

// create and return the single instance
iAtomSpaceWrapper* ASW()
{
    static iAtomSpaceWrapper* instance;
    if (instance == NULL) {    
        LOG(2, "Creating AtomSpaceWrappers...");
#if LOCAL_ATW
        instance = &LocalATW::getInstance();
#else
        DirectATW::getInstance();
        instance = &NormalizingATW::getInstance();
#endif
    }

    return instance;
}

/////
// NormalizingATW methods
/////
NormalizingATW::NormalizingATW()
{
}

//#define BL 2
pHandle NormalizingATW::addLink(Type T, const pHandleSeq& hs,
        const TruthValue& tvn,bool fresh,bool managed)
{
    AtomSpace *a = AS_PTR;
    pHandle ret= PHANDLE_UNDEFINED;

    bool ok_forall=false;

    char buf[500];
    sprintf(buf, "Adding link of type %s (%d)", type2name[T].c_str(), T);
    LOG(4, buf);

    if (hs.size() > 7)
    {
        LOG(4, "Adding large-arity link!");
/*      if (TheLog.getLevel()>=5)
        {
            char t[100];
            gets(t);
        }*/
    }

#if 0
    if (T == IMPLICATION_LINK
        && hs.size()==2
        && inheritsType(a->getType(hs[0]), FALSE_LINK))
    {
        assert(hs.size()==2 || hs.empty());
        
        ret = hs[1];
    }
    else if (T == IMPLICATION_LINK
        && hs.size()==2
        && inheritsType(a->getType(hs[1]), FALSE_LINK))
    {
        assert(hs.size()==2 || hs.empty());

        HandleSeq NOTarg;
        NOTarg.push_back(hs[0]);
        
        ret = addLink(NOT_LINK, NOTarg, TruthValue::TRUE_TV(), fresh, managed);
    }
    else if (T == IMPLICATION_LINK                              //Accidentally similar to da above
            && hs.size()==2
            && inheritsType(a->getType(hs[0]), AND_LINK)
            && containsNegation(hs[0], hs[1]))
    {
        HandleSeq NOTarg;
        NOTarg.push_back(hs[0]);

        ret = addLink(NOT_LINK, NOTarg, TruthValue::TRUE_TV(), fresh, managed);
    }
    else if (T == IMPLICATION_LINK
        && !hs.empty()
        && inheritsType(a->getType(hs[1]), AND_LINK))
    {
        assert(hs.size()==2 || hs.empty());

        LOG(BL, "Cut A=>AND(B,C,...) into AND(A=>B, A=>C, ...)");
        
        HandleSeq imps;
        
        HandleSeq hs2 = a->getOutgoing(hs[1]);
        for (int i = 0; i < hs2.size(); i++)
        {
            HandleSeq new_hs;
            new_hs.push_back(hs[0]);
            new_hs.push_back(hs2[i]);
            
            imps.push_back( addLink(T, new_hs, tvn,fresh,managed) );
        }
        
        ret = addLink(AND_LINK,imps,TruthValue::TRUE_TV(),fresh,managed);
    }
    else
#endif
#if 0
    if (T == IMPLICATION_LINK
        && !hs.empty()
        && inheritsType(a->getType(hs[0]), IMPLICATION_LINK))
    {
        assert(hs.size()==2 || hs.empty());

        HandleSeq hs2 = a->getOutgoing(hs[0]);

        Handle c = hs[1];
        Handle a = hs2[0];
        Handle b = hs2[1];

//      LOG(BL, "=>( =>(A,B), C) ) into =>( |(B,~A),C ) into  =>( ~&(~B,A),C ) ");
        LOG(BL, "=>( =>(A,B), C) ) into &( =>( A&B, C), =>(~A, C) )");

        HandleSeq NOTa_args;
        NOTa_args.push_back(a);
        
        HandleSeq AND_args;
        AND_args.push_back(a);
        AND_args.push_back(b);

//      HandleSeq NOTAND_args;

        HandleSeq imps1, imps2;
        imps1.push_back(addLink(AND_LINK, AND_args, TruthValue::TRUE_TV(), fresh,managed));
        imps1.push_back(c);

        imps2.push_back(addLink(NOT_LINK, NOTa_args, TruthValue::TRUE_TV(), fresh,managed));
        imps2.push_back(c);
        
        HandleSeq new_hs;
        new_hs.push_back(addLink(IMPLICATION_LINK, imps1, TruthValue::TRUE_TV(), fresh,managed));
        new_hs.push_back(addLink(IMPLICATION_LINK, imps2, TruthValue::TRUE_TV(), fresh,managed));
        
        ret = addLink(AND_LINK,new_hs,TruthValue::TRUE_TV(),fresh,managed);
    }
#endif
/*  else if (T == AND_LINK
            && hs.size()==2
            && a->getType(hs[1]) != IMPLICATION_LINK)
    {
        LOG(0, "AND => Implication");
        cprintf(0,"%d\n",nm->getType(hs[1]));
        getc(stdin);
        
        TruthValue **tvs = new TruthValue *[3];
        tvs[0] = &tvn;
        tvs[1] = getTV(hs[0]);
        tvs[2] = getTV(hs[1]);
        
        TruthValue *impTV = ImplicationConstructionFormula().compute(tvs,3);

        HandleSeq new_hs;
        new_hs.push_back(addLink(T,hs,tvn,fresh,managed));
        new_hs.push_back(addLink(IMPLICATION_LINK,hs,impTV,fresh,managed));
        
        cprintf(0,"EEE %d\n",nm->getType(new_hs[1]));
        
        ret = addLink(AND_LINK, new_hs, TruthValue::TRUE_TV(), fresh,managed);

        LOG(0, "Adding AND-reformed:");
        printTree(ret, 0, 0);
        getc(stdin);
    }*/
    
#if 0
    else if (T == NOT_LINK
            && !hs.empty()
            && inheritsType(a->getType(hs[0]), NOT_LINK)
            && tvn.getMean() > 0.989
            && binaryTrue(hs[0]) )
    {
        LOG(BL, "~~A <---> A");

        HandleSeq relevant_args = a->getOutgoing(hs[0]);
        assert(relevant_args.size() == 1);
        Type relevant_type = a->getType(relevant_args[0]);

        //if (tvn.getMean() > 0.989 && binaryTrue(relevant_args[0]) )
        ret = a->getOutgoing(hs[0])[0]; //addLink(relevant_type, relevant_args,TruthValue::TRUE_TV(),fresh);
    }
    else if (T == AND_LINK
            && !hs.empty()
            && tvn.getMean() > 0.989
            && getFirstIndexOfType(hs, AND_LINK) >= 0
            && binaryTrue(hs[getFirstIndexOfType(hs, AND_LINK)]) )
            //&& inheritsType(a->getType(hs[0]), AND_LINK))
    {
        LOG(BL, "AND(AND(B), AND(A)) <---> AND(B,A)");

//      bool all_true_and = true;

        HandleSeq new_args;

        for (int ii = 0; ii < hs.size(); ii++)
        {
//          printTree(hs[ii],0,4);

            if (inheritsType(a->getType(hs[ii]), AND_LINK)
                && binaryTrue(hs[ii]))
            {
                HandleSeq args1 = a->getOutgoing(hs[ii]);

                for (int a = 0; a < args1.size(); a++)
                    new_args.push_back(args1[a]);
            }
            else
                new_args.push_back(hs[ii]);
        }

        ret = addLink(AND_LINK, new_args, tvn, fresh,managed);
#if P_DEBUG
        LOG(4, "Adding AND-reformed:");
        printTree(ret, 0, 4);
#endif
    }
    else if (T == AND_LINK
            && hs.size()==1
            && tvn.getMean() > 0.989)
    {
        LOG(BL, "AND(A) <---> A");

        ret = hs[0];
    }
    else if (T == AND_LINK)
    {
//for (int a=0;a<hs.size(); a++)
//  printTree(hs[a],0,4);

        LOG(BL, "&(..., A, ..., ~A, ...) => Falsum");

        if (hasFalsum(hs))
            ret = addNode(FALSE_LINK, "FALSE", tvn, fresh,managed);
        else
        {
            LOG(BL, "AND(A,B,A) <---> AND(A,B)");

            int original_size = hs.size();
            remove_redundant(hs);

            if (hs.size() != original_size) //Changed.
            {
                LOG(5, "CHANGES FROM AND(A,B,A) <---> AND(A,B)");

                ret = addLink(AND_LINK, hs, tvn, fresh,managed);
    
//printTree(ret,0,5);
    
                LOG(5, "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
            }
            else
            {
                LOG(5, "NO CHANGE FROM AND(A,B,A) <---> AND(A,B)");
            }
        }
    }
    else if (T == NOT_LINK
            && !hs.empty()
            && inheritsType(a->getType(hs[0]), IMPLICATION_LINK))
    {
        LOG(BL, "~(A=>B) <---> ~(B | ~A) <---> &(A,~B)");

        HandleSeq IMP_args = a->getOutgoing(hs[0]);
        assert(IMP_args.size() == 2);
    
        HandleSeq not_arg;
        not_arg.push_back(IMP_args[1]);

        HandleSeq new_args;
        new_args.push_back(IMP_args[0]);
        new_args.push_back(addLink(NOT_LINK, not_arg,TruthValue::TRUE_TV(),fresh,managed));

        ret = addLink(AND_LINK, new_args, tvn, fresh,managed);
    }
    else if (T == NOT_LINK
            && !hs.empty()
            && inheritsType(a->getType(hs[0]), AND_LINK))
    {
        LOG(BL, "Cut -AND(B,C,...) into AND(B=>-C, C=>-B, ...)");
        
        HandleSeq imps;
        
        HandleSeq and_args = a->getOutgoing(hs[0]);

        for (int i = 0; i < and_args.size(); i++)
        {
            HandleSeq new_and_args;
            cutVector<Handle>(and_args, i, new_and_args);

            HandleSeq new_imp_args;
            HandleSeq not_arg;
            not_arg.push_back(and_args[i]);

            new_imp_args.push_back(addLink(AND_LINK, new_and_args,TruthValue::TRUE_TV(), fresh,managed));
            new_imp_args.push_back(addLink(NOT_LINK, not_arg, TruthValue::TRUE_TV(), fresh,managed));                       
        
            imps.push_back( addLink(IMPLICATION_LINK, new_imp_args, TruthValue::TRUE_TV(), fresh,managed) );
        }
        
        ret = addLink(AND_LINK, imps, TruthValue::TRUE_TV(), fresh,managed);
    } 
#endif
/*  else if (T == IMPLICATION_LINK
            && hs.size()==2
            && tvn.getMean() > 0.989
            && binaryTrue(hs[1])
            && inheritsType(a->getType(hs[1]), IMPLICATION_LINK))
    {
        LOG(0, "(A=>(B=>C)) --->    ((A&B) => C)");
//exit(0);
        Handle old_head = hs[0];
        HandleSeq old_imp_args = a->getOutgoing(hs[1]);
        Handle tail = old_imp_args[1];

        HandleSeq new_and_args;

        new_and_args.push_back(old_head);
        new_and_args.push_back(old_imp_args[0]);

        HandleSeq new_imp_args;
        new_imp_args.push_back(addLink(AND_LINK, new_and_args, TruthValue::TRUE_TV(), fresh,managed));
        new_imp_args.push_back(tail);
        
        ret = addLink(IMPLICATION_LINK, new_imp_args, TruthValue::TRUE_TV(), fresh,managed);

//      printTree(ret);
    }*/

#if 0
    else if (T == IMPLICATION_LINK
            && !hs.empty()
            && inheritsType(a->getType(hs[0]), AND_LINK)
            && getFirstIndexOfType(a->getOutgoing(hs[0]), IMPLICATION_LINK) >= 0)
    {
        LOG(1, "Cut (A& (B=>C) ) => D   --->    (C & A) => D   &   (~B & A)=>D");

        assert(hs.size()==2);

        HandleSeq old_and_args = a->getOutgoing(hs[0]);
        Handle tail = hs[1];

//printTree(hs[1],0,1);

        int imp_index = getFirstIndexOfType(old_and_args, IMPLICATION_LINK);

/*LOG(1, "Old HS");
for (int a2=0;a2<hs.size(); a2++)
    printTree(hs[a2],0,1);

LOG(1, "Old AND");
for (int a1=0;a1<old_and_args.size(); a1++)
    printTree(old_and_args[a1],0,1);*/

        HandleSeq new_and_args1, new_and_args2;
        cutVector<Handle>(old_and_args, imp_index, new_and_args1);
        new_and_args2 = new_and_args1;

        HandleSeq internal_imp_args = a->getOutgoing(old_and_args[imp_index]);
        HandleSeq not_arg;
        not_arg.push_back(internal_imp_args[0]);
        
        new_and_args1.push_back(internal_imp_args[1]);
        new_and_args2.push_back(addLink(NOT_LINK, not_arg, TruthValue::TRUE_TV(), fresh,managed));

#if P_DEBUG
int a;
LOG(1, "New AND");
for (a=0;a<new_and_args1.size(); a++)
    printTree(new_and_args1[a],0,1);
#endif

        HandleSeq new_imp_args1, new_imp_args2;
        new_imp_args1.push_back(addLink(AND_LINK, new_and_args1, TruthValue::TRUE_TV(), fresh,managed));
        new_imp_args1.push_back(tail);
        new_imp_args2.push_back(addLink(AND_LINK, new_and_args2, TruthValue::TRUE_TV(), fresh,managed));
        new_imp_args2.push_back(tail);

#if P_DEBUG
LOG(1, "New IMP1");
for (a=0;a<new_imp_args1.size(); a++)
    printTree(new_imp_args1[a],0,1);
#endif

#if P_DEBUG
LOG(1, "New IMP2");
for (a=0;a<new_imp_args2.size(); a++)
    printTree(new_imp_args2[a],0,1);
#endif

        HandleSeq new_main_and_args;
        new_main_and_args.push_back(addLink(IMPLICATION_LINK, new_imp_args1, TruthValue::TRUE_TV(), fresh,managed));
        new_main_and_args.push_back(addLink(IMPLICATION_LINK, new_imp_args2, TruthValue::TRUE_TV(), fresh,managed));
        
        ret = addLink(AND_LINK, new_main_and_args, TruthValue::TRUE_TV(), fresh,managed);

#if P_DEBUG
LOG(1, "Collapsed:");
printTree(ret,0,1);
#endif

    }
#endif
    //! @todo Should ExtensionalEquivalenceLinks also be converted
    //! to ExtensionalImplicationLinks?
    else if (T == EQUIVALENCE_LINK && hs.size()==2)
    //else if (hs.size()==2)
    {
        // Convert EQUIVALENCE_LINK into two IMPLICATION_LINKs
        // that are mirrored and joined by an AND_LINK.
        const pHandleSeq EquiTarget = hs;
        pHandleSeq ImpTarget1, ImpTarget2;
        
        for (pHandleSeq::const_iterator i = EquiTarget.begin(); i != EquiTarget.end(); i++)
            ImpTarget1.push_back((*i));

        int zz=ImpTarget1.size();
        assert(ImpTarget1.size()==2);
        
        for (pHandleSeq::const_reverse_iterator j = EquiTarget.rbegin(); j != EquiTarget.rend(); j++)
            ImpTarget2.push_back((*j));
        
        assert(ImpTarget2.size()==2);
        
//      const TruthValue& outerTV = getTV(exL);
        
        pHandleSeq ANDargs;

        ANDargs.push_back( addLink(IMPLICATION_LINK, ImpTarget1,
            tvn,
            true,managed));

        ANDargs.push_back( addLink(IMPLICATION_LINK, ImpTarget2,
            tvn,
            true,managed));

//      reverse(ANDargs.begin(), ANDargs.end());

        ret = addLink(AND_LINK, ANDargs, TruthValue::TRUE_TV(), fresh,managed);
    }
    else if (T == FORALL_LINK
            && hs.size() == 2
            && inheritsType(getType(hs[1]), AND_LINK)
            && binaryTrue(hs[1])
            && getArity(hs[1]) > 1)
    {
        // FORALL quantifier with AND_LINK is expanded into a LIST of FORALL
        // for each component within the AND.
        unsigned int AND_arity = getArity(hs[1]);

        pHandleSeq fa_list;

        for (unsigned int i = 0; i < AND_arity; i++)
        {
            pHandleSeq fora_hs;
            // How come for all links need the source to be freshened?
            // Probably no longer required... but actually, this freshens
            // all the variables for the forall link... such that each link has
            // it's own variable nodes.
            fora_hs.push_back(hs[0]);//freshened(hs[0],managed));
            fora_hs.push_back(getOutgoing(hs[1],i));

            // fresh parameter should probably be set as true, since above
            // freshened links/nodes will not have any links from them yet.
            fa_list.push_back( addLink(FORALL_LINK, fora_hs, tvn,
                        fresh, managed) );
        }

        assert(fa_list.size() == AND_arity);

        ret = addLink(LIST_LINK, fa_list, TruthValue::TRUE_TV(), fresh,managed);

//printTree(ret,0,0);
    }
#if  0
    else if (T==OR_LINK)
//          && tvn.getMean() > 0.989)
    {
        LOG(BL, "OR(A,B) <---> ~(~A AND ~B)");

        HandleSeq and_args;
        for (int i = 0; i < hs.size(); i++)
        {
            HandleSeq not_arg;
            not_arg.push_back(hs[i]);
            and_args.push_back(addLink(NOT_LINK, not_arg,TruthValue::TRUE_TV(),fresh,managed));
        }
    
        HandleSeq not_arg;
        not_arg.push_back(addLink(AND_LINK, and_args, tvn, fresh,managed));

        ret = addLink(NOT_LINK, not_arg,TruthValue::TRUE_TV(),fresh,managed);
    }
/*  else if (T==FORALL_LINK && hs.size()==2)
    {
ok_forall=true;
    }*/

/*  else if (AddToU
            && !inheritsType(T, FORALL_LINK))
    {
        HandleSeq emptys, forall_args;

        forall_args.push_back( addLink(LIST_LINK, emptys, TruthValue::TRUE_TV(), false,managed) );
        forall_args.push_back( addLink(T, hs, tvn, fresh,managed) );

        ret = addLink(FORALL_LINK, forall_args, TruthValue::TRUE_TV(), fresh,managed);
    }*/
#endif
    if (ret==PHANDLE_UNDEFINED)
    {
//      if (symmetricLink(T))
//          remove_if(hs.begin(), hs.end(), isEmptyLink);

/*      if (T==AND_LINK)
        {

            int original_size = hs.size();
            remove_redundant(hs);
            assert (hs.size() == original_size);
        }*/

        LOG(5, "Adding to Core...");

        ret = FIMATW::addLink(T,hs,tvn,fresh,managed);
        //ret = a->addLink(T,hs,tvn,fresh,managed);

        LOG(5, "Added.");

#if P_DEBUG
        if (T == IMPLICATION_LINK && hs.size()==2 && a->getType(hs[1])==AND_LINK )
            printTree(ret, 0, 4);
#endif
    }
    return ret;
}

pHandle NormalizingATW::addNode(Type T, const string& name, const TruthValue& tvn,bool fresh,bool managed)
{
    //Handle ret = a->addNode(T, name, tvn, fresh);
    pHandle ret = FIMATW::addNode(T, name, tvn, fresh,managed);
    return ret;
}

// LocalATW
#if 0
LocalATW::LocalATW()
: capacity(0)
{
puts("Loading classed to LocalATW..."); 
    for (int i = 0; i < NUMBER_OF_CLASSES; i++)
    {
        mindShadowMap[i] = shared_ptr<set<Handle> >(new set<Handle>);
        for (   HandleEntry* e = a->getHandleSet((Type)i, true);
                e && e->handle;
                e = e->next)
        {
        printf(".\n");
            mindShadowMap[i]->insert(e->handle);
        }
            
//      delete e;
    }   
}

shared_ptr<set<Handle> > LocalATW::getHandleSet(Type T, const string& name, bool subclass)
{
    assert(!subclass); ///Not supported (yet)
    return mindShadowMap[T];
}

bool equal_vectors(Handle* lhs, int lhs_arity, const vector<Handle>& rhs)
{
    if (lhs_arity != rhs.size())
        return false;
    for (int i=0; i< lhs_arity; i++)
        if (lhs[i] != rhs[i])
            return false;
        
    return true;
}

bool LocalATW::inHandleSet(Type T, const string& name, shared_ptr<set<Handle> > res, Handle* ret)
{
    for (set<Handle>::iterator i=res->begin(); i != res->end(); i++)
        if (inheritsType((*i)->getType(),NODE)
            &&  ((Node*)(*i))->getName() == name)
        {
            if (ret)
                *ret = *i;
            return true;
        }   

    return false;   
}

void LocalATW::SetCapacity(unsigned long atoms)
{
    capacity = atoms;
}

bool LocalATW::inHandleSet(Type T, const HandleSeq& hs, shared_ptr<set<Handle> > res, Handle* ret)
{
    const int N = hs.size();
    
    for (set<Handle>::iterator i=res->begin(); i != res->end(); i++)
        if ((*i)->getArity() == N &&
             equal_vectors((*i)->getOutgoingSet(),
                            N,
                            hs))
        {
            if (ret)
                *ret = *i;
            return true;
        }   

    return false;   
}

void LocalATW::ClearAtomSpace()
{
    int count=0;
    for (map<Type, shared_ptr<set<Handle> > >::iterator m=mindShadowMap.begin();
        m!= mindShadowMap.end(); m++)
    {
        for (set<Handle>::iterator i = m->second->begin(); i!=m->second->end(); i++)
        {
            delete *i;
            count++;
        }
        m->second->clear();
    }   
cprintf(1, "%d entries freed (%.2f Mb).\n", count, (count * sizeof(Link))/1048576.0 );
}



void LocalATW::DumpCore(Type T)
{
    if (T == 0)
    {
        for (map<Type, shared_ptr<set<Handle> > >::iterator m=mindShadowMap.begin();
            m!= mindShadowMap.end(); m++)
        {   
            if (m->first) //Type #0 is not printed out.
                DumpCore(m->first);
        }
    }
    else
        for (set<Handle>::iterator i = mindShadowMap[T]->begin();
             i!=mindShadowMap[T]->end(); i++)
        {
            printf("[%d]\n", *i);
            printTree(*i,0,0);
        }
}                                   

/// Ownership of tvn is given to this method

Handle LocalATW::addLink(Type T, const HandleSeq& hs, const TruthValue& tvn, bool fresh, bool managed)
{
    Handle ret = NULL;
    
    if (fresh || !inHandleSet(T,hs,mindShadowMap[T],&ret))
    {
        if (capacity && mindShadowMap[T]->size() >= capacity && !q[T].empty())
        {
            LOG(2, "Above capacity, removing...");
            
//          set<Handle>::iterator remo = mindShadowMap[T]->begin();
            set<Handle>::iterator remo = q[T].front();
            if (*remo)
                delete *remo;
            mindShadowMap[T]->erase(remo);
            
            q[T].pop();
        }
        
        /// Ownership of tvn is passed forward
        
        Link* link = new Link( T,  hs, tvn);
        pair<set<Handle>::iterator, bool> si = mindShadowMap[T]->insert(link);
        if (managed)
            q[T].push(si.first);
        return link;
    }
    else
    {
        return ret;
    }
}

Handle LocalATW::addNode(Type T, const std::string& name, const TruthValue& tvn, bool fresh, bool managed)
{
LOG(4,"Adding node.."); 

    Handle ret = NULL;
    
    if (fresh || !inHandleSet(T,name,mindShadowMap[T],&ret))
    {
        if (capacity && mindShadowMap[T]->size() >= capacity && !q[T].empty())
        {
            LOG(2, "Above capacity, removing...");

            //set<Handle>::iterator remo = mindShadowMap[T]->begin();
            set<Handle>::iterator remo = q[T].front();
            if (*remo)
                delete *remo;
            mindShadowMap[T]->erase(remo);
            
            q[T].pop();
        }

        Node* node = new Node( T,  name,  tvn); 
        pair<set<Handle>::iterator, bool> si = mindShadowMap[T]->insert(node);
        if (managed)
            q[T].push(si.first);
LOG(4,"Node add ok.");      
        return node;
    }
    else
    {
LOG(4,"Node add ok.");              
        return ret;
    }
}

#endif

/////
// FIMATW methods
/////
pHandle FIMATW::addNode(Type T, const string& name, const TruthValue& tvn, bool fresh,bool managed)
{
    AtomSpace *a = AS_PTR;
/// The method should be, AFAIK, identical to the one in DirectATW, unless FIM is actually in use.
    
    assert(!tvn.isNullTv());
    
//  assert(haxx::AllowFW_VARIABLENODESinCore || T != FW_VARIABLE_NODE);

    /// Disable confidence for variables:

#ifdef USE_PSEUDOCORE
    const TruthValue& tv = SimpleTruthValue(tvn.getMean(), 0.0f);
    const TruthValue& mod_tvn = (!inheritsType(T, VARIABLE_NODE))? tvn : tv; 

    pHandle ret = addNode( T, name, mod_tvn, fresh,managed);
#else
    
LOG(3,"FIMATW::addNode");

    if (inheritsType(T, FW_VARIABLE_NODE))
    {
        /// Safeguard the identity of variables.
    
        map<string,pHandle>::iterator existingHandle = haxx::variableShadowMap.find(name);
        if (existingHandle != haxx::variableShadowMap.end())
            return existingHandle->second;
        
    }   

    Node node( T,  name,  tvn);
LOG(3,"FIMATW: addAtomDC"); 
    // fresh bug, done?
    pHandle ret = addAtomDC(node,fresh,managed);

#if HANDLE_MANAGEMENT_HACK
LOG(3,"FIMATW: addAtomDC OK, checking for Handle release needed");      
    /// haxx:: // Due to core relic
    if (ret != node)
        delete node;
#endif
    
    LOG(3, "Add ok.");
    
#endif

    if (inheritsType(T, FW_VARIABLE_NODE))
        haxx::variableShadowMap[name] = ret;
    
#if USE_MIND_SHADOW
haxx::mindShadow.push_back(ret);    
haxx::mindShadowMap[T].push_back(ret);
#endif


/*  if (PLN_CONFIG_FIM)
    {
        unsigned int *pat1 = new unsigned int[PLN_CONFIG_PATTERN_LENGTH];
        myfim._copy(pat1, myfim.zeropat);
        pat1[0] = (unsigned int)ret;
        myfim.add(pat1);
    }*/

    return ret;
}

pHandle FIMATW::addLink(Type T, const pHandleSeq& hs, const TruthValue& tvn, bool fresh,bool managed)
{
    pHandle ret = directAddLink(T, hs, tvn, fresh,managed);  

#if 0
    if (PLN_CONFIG_FIM)
    {
        atom a(ret);
        unsigned int *pat1 = new unsigned int[PLN_CONFIG_PATTERN_LENGTH];
        try {
            a.asIntegerArray(pat1, PLN_CONFIG_PATTERN_LENGTH, node2pat_id, next_free_pat_id);
        } catch(string s) { LOG(4, s); return ret; }
        myfim.add((unsigned int)ret, pat1);
    }
#endif

    return ret; 

}

/////
// DirectATW methods
/////
DirectATW::DirectATW()
{
}

pHandle DirectATW::addLink(Type T, const pHandleSeq& hs, const TruthValue& tvn, bool fresh, bool managed)
{
    return directAddLink(T, hs, tvn, fresh,managed);
}

pHandle DirectATW::addNode(Type T, const string& name, const TruthValue& tvn, bool fresh,bool managed)
{
    AtomSpace *a = AS_PTR;
    assert(!tvn.isNullTv());
    
    //assert(haxx::AllowFW_VARIABLENODESinCore || T != FW_VARIABLE_NODE);

    // Disable confidence for variables:
#ifdef USE_PSEUDOCORE
    //const TruthValue& mod_tvn = (!inheritsType(T, VARIABLE_NODE)?
    //  tvn : SimpleTruthValue(tvn.getMean(), 0.0f));
    const TruthValue& tv = SimpleTruthValue(tvn.getMean(), 0.0f); 
    const TruthValue& mod_tvn = (!inheritsType(T, VARIABLE_NODE))? tvn : tv;

    return a->addNode( T, name, mod_tvn, fresh,managed);
#else
    LOG(3,  "DirectATW::addNode");
    assert(1);
        
    if (inheritsType(T, FW_VARIABLE_NODE))
    {
        // Safeguard the identity of variables.
        map<string,pHandle>::iterator existingHandle = haxx::variableShadowMap.find(name);
        if (existingHandle != haxx::variableShadowMap.end())
            return existingHandle->second;
    }   
    Node node( T,  name,  tvn);
    // TODO: add related context
    pHandle ret = addAtomDC(node, fresh, managed);
    LOG(3, "Add ok.");
    
    if (inheritsType(T, FW_VARIABLE_NODE))
        haxx::variableShadowMap[name] = ret;
    
#if USE_MIND_SHADOW
haxx::mindShadow.push_back(ret);    
haxx::mindShadowMap[T].push_back(ret);
#endif
    return ret;
#endif
}

} //~namespace

