/** iostream_combo.cc --- 
 *
 * Copyright (C) 2012 Nil Geisweiller
 *
 * Authors: Nil Geisweiller <ngeiswei@gmail.com>
 *          Matt Chapman <Matt@NinjitsuWeb.com>
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

#include "iostream_combo.h"
#include "procedure_call.h"
#include <boost/range/algorithm/find.hpp>

namespace opencog { namespace combo {

using namespace std;
using namespace boost;

// uncomment this to output a negative literal !$n instead of not($n)
#define ABBREVIATE_NEGATIVE_LITERAL

ostream& ostream_builtin(ostream& out, const builtin& h, format f)
{
    switch(f) {
    case fmt::python:
        switch (h) {
        case id::null_vertex:
            return out << "null_vertex";
        case id::logical_and:
            return out << "ands";
        case id::logical_or:
            return out << "ors";
        case id::logical_not:
            return out << "not";
        case id::logical_true:
            return out << "True";
        case id::logical_false:
            return out << "False";
        default:
            return out << "Builtin: " << (unsigned) h << " unknown";
        }
    case fmt::combo:
        switch (h) {
        case id::null_vertex:
            return out << "null_vertex";
        case id::logical_and:
            return out << "and";
        case id::logical_or:
            return out << "or";
        case id::logical_not:
            return out << "not";
        case id::logical_true:
            return out << "true";
        case id::logical_false:
            return out << "false";
        case id::plus:
            return out << "+";
        case id::times:
            return out << "*";
        case id::div:
            return out << "/";
        case id::log:
            return out << "log";
        case id::exp:
            return out << "exp";
        case id::sin:
            return out << "sin";
        case id::greater_than_zero:
            return out << "0<";
        case id::impulse:
            return out << "impulse";
        case id::rand:
            return out << "rand";
        case id::list:
            return out << "list";
        case id::car:
            return out << "car";
        case id::cdr:
            return out << "cdr";
        case id::cons:
            return out << "cons";
        case id::contin_if:
            return out << "contin_if";
        case id::cond:
            return out << "cond";
        case id::equ:
            return out << "equ";
        default:
            return out << "Builtin " << (unsigned) h << " unknown";
        }
    default:
        return out << "Format " << f << " unknown";
    }
}

ostream& ostream_argument(ostream& out, const argument& a, format f)
{
    switch(f) {
    case fmt::python:
        if (a.is_negated())        
            return out << "not(i[" << -a.idx - 1 << "]),";
        return out << "i[" << a.idx - 1 << "],";
    case fmt::combo:
#ifdef ABBREVIATE_NEGATIVE_LITERAL
        return ostream_abbreviate_literal(out, a);
#else
        if (a.is_negated())        
            return out << "not($" << -a.idx << ")";
        return out << "$" << a.idx << vm;
#endif
    default:
        return out << "Format " << f << "unknown";
    }    
}

ostream& ostream_vertex(ostream& out, const vertex& v, format f)
{
    // Handle the most likely types first.
    if (const argument* a = get<argument>(&v))
        return ostream_argument(out, *a, f);
    if (const builtin* h = get<builtin>(&v))
        return ostream_builtin(out, *h, f);
    if (const enum_t* m = get<enum_t>(&v))
        return out << m->getContent();

    // XXX ?? Ahem, won't calling out<<(*m) just lead to infinite
    // recursion ?? 
    if (const ann_type* z = get<ann_type>(&v))
        return out << (*z);
    if (const wild_card* w = get<wild_card>(&v))
        return out << (*w);
    if (const action* act = get<action>(&v))
        return out << (*act);
    if (const builtin_action* aact = get<builtin_action>(&v))
        return out << (*aact);
    if (const perception* per = get<perception>(&v))
        return out << (*per);
    if (const indefinite_object* iot = get<indefinite_object>(&v))
        return out << (*iot);
    if (const message* m = get<message>(&v))
        return out << m->getContent();
    if (const definite_object* dot = get<definite_object>(&v))
        return out << (*dot);
    if (const action_symbol* as = get<action_symbol>(&v))
        return out << (*as);
    if (const procedure_call* cp = get<procedure_call>(&v))
        return out << (*cp);

    try {
        return out << get<contin_t>(v);
    } catch (...) {
        OC_ASSERT(false, "Don't know how to print this type");
    }

    return out;
}

std::ostream& ostream_combo_tree(std::ostream& out, const combo_tree ct, format f) {
    for (combo_tree::iterator it=ct.begin(); it!=ct.end(); ++it) {
        ostream_combo_it(out, it, f);
        it.skip_children();
        out << " ";
    }
    return out;
}
        
string ph2l(const string& ce, const vector<string>& labels)
{
    /// @todo the implementation could be done in 2 lines with
    /// boost.regex with boost version 1.42 or above because then we
    /// can use Formatter as callback, but we're stuck with boost 1.38
    /// :-(
    string res;
    string match;
    bool matching = false;
    foreach(char c, ce) {
        if(!matching) {
            res += c;
            if(c == '$') // matching starts
                matching = true;
        } else {
            if(c == ' ' || c == ')' || c == '\n') { //matching ends
                res += labels[lexical_cast<arity_t>(match) - 1] + c;
                match.clear();
                matching = false;
            } else // matching goes
                match += c;
        }
    }
    // if a matching is going on flush to the result
    if(matching)
        res += labels[lexical_cast<arity_t>(match) - 1];
    return res;
}

string l2ph(const string& ce, const vector<string>& labels)
{
    /// @todo the implementation could be done in 2 lines with
    /// boost.regex with boost version 1.42 or above because then we
    /// can use Formatter as callback, but we're stuck with boost 1.38
    /// :-(
    string res;
    string match;
    bool matching = false;
    foreach(char c, ce) {
        if(!matching) {
            res += c;
            if(c == '$') // matching starts
                matching = true;
        } else {
            if(c == ' ' || c == ')' || c == '\n') { //matching ends
                auto found_it = find(labels, match);
                OC_ASSERT(found_it != labels.end(), "No label %s matching",
                          match.c_str());
                arity_t idx = distance(labels.begin(), found_it) + 1;
                res += lexical_cast<string>(idx) + c;
                match.clear();
                matching = false;
            } else // matching goes
                match += c;
        }
    }
    // if a matching is going on flush to the result
    if(matching) {
        auto found_it = boost::find(labels, match);
        OC_ASSERT(found_it != labels.end(), "No label %s matching",
                  match.c_str());
        arity_t idx = distance(labels.begin(), found_it) + 1;
        res += lexical_cast<string>(idx);
    }
    return res;
}

ostream& operator<<(ostream& out, const ann_type& h)
{
    switch (h.id) {
    case id::ann:
        return out << "ann";
    case id::ann_input:
        return out << "$I" << h.idx;
    case id::ann_node:
        return out << "$N" << h.idx;
    default:
        return out << "ANN : UNKNOWN_HANDLE";
   }
}

ostream& operator<<(ostream& out, const builtin& h)
{
    return ostream_builtin(out, h, fmt::combo);
}

ostream& operator<<(ostream& out, const wild_card& w)
{
    switch (w) {
    case id::asterisk:
        return out << "_*_";
    default:
        return out << "WILD_CARD: UNKNOWN_HANDLE";
    }
}

ostream& ostream_abbreviate_literal(ostream& out, const argument& a) {
    if(a.is_negated()) {
        return out << "!$" << -a.idx;
    }
    return out << "$" << a.idx;
}

ostream& operator<<(ostream& out, const argument& a)
{
    return ostream_argument(out, a, fmt::combo);
}

ostream& operator<<(ostream& out, const vertex& v)
{
    return ostream_vertex(out, v, fmt::combo);
}

}} // ~namespaces combo opencog
