/** metapopulation.h ---
 *
 * Copyright (C) 2010 Novemente LLC
 * Copyright (C) 2012 Poulin Holdings
 *
 * Authors: Nil Geisweiller, Moshe Looks, Linas Vepstas
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


#ifndef _OPENCOG_METAPOPULATION_H
#define _OPENCOG_METAPOPULATION_H

#include <future>
#include <math.h>

#include <boost/unordered_set.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/irange.hpp>

#include <opencog/util/selection.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/numeric.h>
#include <opencog/util/functional.h>
#include <opencog/util/algorithm.h>
#include <opencog/util/oc_omp.h>

#include <opencog/comboreduct/combo/combo.h>
#include <opencog/comboreduct/reduct/reduct.h>

#include "../representation/instance_set.h"
#include "../representation/representation.h"
#include "scoring.h"
#include "types.h"
#include "feature_selector.h"

#define EVALUATED_ALL_AVAILABLE 1234567

namespace opencog {
namespace moses {

using std::pair;
using std::make_pair;
using boost::logic::tribool;
using boost::logic::indeterminate;
using namespace combo;

static const operator_set empty_ignore_ops = operator_set();

/**
 * parameters about deme management
 */
struct metapop_parameters
{
    metapop_parameters(int _max_candidates = -1,
                       bool _reduce_all = true,
                       bool _revisit = false,
                       bool _include_dominated = true,
                       score_t _complexity_temperature = 3.0f,
                       const operator_set& _ignore_ops = empty_ignore_ops,
                       bool _enable_cache = false,
                       unsigned _jobs = 1,
                       const combo_tree_ns_set* _perceptions = NULL,
                       const combo_tree_ns_set* _actions = NULL,
                       const feature_selector* _fstor = NULL) :
        max_candidates(_max_candidates),
        reduce_all(_reduce_all),
        revisit(_revisit),
        include_dominated(_include_dominated),
        use_diversity_penalty(false), // XXX should pass as arg ...
        complexity_temperature(_complexity_temperature),
        ignore_ops(_ignore_ops),
        enable_cache(_enable_cache),
        jobs(_jobs),
        perceptions(_perceptions),
        actions(_actions),
        merge_callback(NULL),
        callback_user_data(NULL),
        fstor(_fstor)
        {}

    // The max number of candidates considered to be added to the
    // metapopulation, if negative then all candidates are considered.
    int max_candidates;

    // If true then all candidates are reduced before evaluation.
    bool reduce_all;

    // When true then visited exemplars can be revisited.
    bool revisit;

    // Ignore behavioral score domination when merging candidates in
    // the metapopulation.  Keeping dominated candidates improves
    // performance by avoiding local maxima.
    bool include_dominated;

    // Enable forced diversification of the metapop.
    bool use_diversity_penalty;

    // Boltzmann temperature ...
    score_t complexity_temperature;

    // the set of operators to ignore
    operator_set ignore_ops;

    // Enable caching of scores.
    bool enable_cache;

    // Number of jobs for metapopulation maintenance such as merging
    // candidates to the metapopulation.
    unsigned jobs;

    // the set of perceptions of an optional interactive agent
    const combo_tree_ns_set* perceptions;
    // the set of actions of an optional interactive agent
    const combo_tree_ns_set* actions;

    bool (*merge_callback)(bscored_combo_tree_set&, void*);
    void *callback_user_data;

    const feature_selector* fstor;
};

/**
 * The metapopulation will store the expressions (as scored trees)
 * that were encountered during the learning process.  Only the
 * highest-scoring trees are typically kept.
 *
 * The metapopulation is updated in iterations. In each iteration, one
 * of its elements is selected as an exemplar. The exemplar is then
 * decorated with knobs and optimized, to create a new deme.  Members
 * of the deme are then folded back into the metapopulation.
 *
 * NOTE:
 *   CScoring = scoring function (output composite scores)
 *   BScoring = behavioral scoring function (output behaviors)
 */
template<typename CScoring, typename BScoring, typename Optimization>
struct metapopulation : bscored_combo_tree_set
{
    typedef metapopulation<CScoring, BScoring, Optimization> self;
    typedef bscored_combo_tree_set super;
    typedef super::value_type value_type;

    // The goal of using unordered_set here is to have O(1) access time
    // to see if a combo tree is in the set, or not.
    typedef boost::unordered_set<combo_tree,
                                 boost::hash<combo_tree> > combo_tree_hash_set;

    // Init the metapopulation with the following set of exemplars.
    void init(const std::vector<combo_tree>& exemplars)
    {
        metapop_candidates candidates;
        foreach (const combo_tree& base, exemplars) {
            combo_tree si_base(base);
            (*simplify_candidate)(si_base);

            penalized_behavioral_score pbs(_bscorer(si_base));
            // XXX Compute the bscore a second time.   The first time
            // was immediately above.  We do it again, because the
            // caching scorer lacks the correct signature.
            // composite_score csc(_cscorer (pbs, tree_complexity(si_base)));
            composite_score csc(_cscorer(si_base));

            candidates[si_base] = composite_behavioral_score(pbs, csc);
        }

        bscored_combo_tree_set mps(candidates.begin(), candidates.end());
        update_best_candidates(mps);
        merge_candidates(mps);
    }

    /**
     *  Constuctor for the class metapopulation
     *
     * @param bases   Exemplars used to initialize the metapopulation
     * @param tt      Type signature of expression to be learned.
     *                That is, the expression must have this signature,
     *                relating the argument variables, and the output type.
     * @param si_ca   Reduct rule for reducing candidate combo trees.
     * @param si_kb   Reduct rule for reducing trees decorated with
     *                knobs.
     * @param sc      Function for scoring combo trees.
     * @param bsc     Behavior scoring function
     * @param opt     Algorithm that find best knob settings for a given
     *                exemplar decorated with knobs.
     * @param pa      Control parameters for this class.
     */
    metapopulation(const std::vector<combo_tree>& bases,
                   const type_tree& type_signature,
                   const reduct::rule& si_ca,
                   const reduct::rule& si_kb,
                   const CScoring& sc,
                   const BScoring& bsc,
                   Optimization& opt = Optimization(),
                   const metapop_parameters& pa = metapop_parameters()) :
        _type_sig(type_signature),
        simplify_candidate(&si_ca),
        simplify_knob_building(&si_kb),
        _cscorer(sc),
        _bscorer(bsc), optimize(opt), params(pa),
        _n_evals(0),
        _n_expansions(0),
        _best_cscore(worst_composite_score), _rep(NULL), _deme(NULL)
    {
        init(bases);
    }

    // Like above but using a single base, and a single reduction rule.
    metapopulation(const combo_tree& base,
                   const type_tree& tt,
                   const reduct::rule& si,
                   const CScoring& sc, const BScoring& bsc,
                   Optimization& opt = Optimization(),
                   const metapop_parameters& pa = metapop_parameters()) :
        _type_sig(tt), simplify_candidate(&si),
        simplify_knob_building(&si), _cscorer(sc),
        _bscorer(bsc), optimize(opt), params(pa),
        _n_evals(0), _n_expansions(0),
        _best_cscore(worst_composite_score), _rep(NULL), _deme(NULL)
    {
        std::vector<combo_tree> bases(1, base);
        init(bases);
    }

    ~metapopulation()
    {
        if (_rep) delete _rep;
        if (_deme) delete _deme;
    }

    /**
     * Return reference to the number of evaluations.
     * Its a non-const reference, because distributed moses increments it directly.
     */
    const size_t& n_evals() const
    {
        return _n_evals;
    }

    size_t& n_evals()
    {
        return _n_evals;
    }

    /**
     * Return the best composite score.
     */
    const composite_score& best_composite_score() const
    {
        return _best_cscore;
    }

    /**
     * Return the best score.
     */
    score_t best_score() const
    {
        return get_score(_best_cscore);
    }

    /**
     * Return the set of candidates with the highest composite
     * scores.  These will all have the the same "best_composite_score".
     */
    const metapop_candidates& best_candidates() const
    {
        return _best_candidates;
    }

    /**
     * Return the best combo tree (shortest best candidate).
     */
    const combo_tree& best_tree() const
    {
        return _best_candidates.begin()->first;
    }

    /**
     * List of exemplars that we've already tried to build reps
     * and demes for.
     */
    const combo_tree_hash_set& visited() const
    {
        return _visited_exemplars;
    }
    combo_tree_hash_set& visited()
    {
        return _visited_exemplars;
    }

    /**
     * Select the exemplar from the population. An exemplar is choosen
     * from the pool of candidates using a Boltzmann distribution
     * exp (-score / temperature).  Thus, they choosen exemplar will
     * typically be high-scoring, but not necessarily the highest-scoring.
     * This allows a range of reasonably-competitive candidates to be
     * explored, and, in practice, prooves to be much more effective
     * than a greedy algo which only selects the highest-scoring candidate.
     *
     * Current experimental evidence shows that temperatures in the
     * range of 6-12 work best for most problems, both discrete
     * (e.g. 4-parity) and conintuous.
     *
     * @return the iterator of the selected exemplar, if no such
     *         exemplar exists then return end()
     */
    const_iterator select_exemplar()
    {
        OC_ASSERT(!empty(), "Empty metapopulation in select_exemplar().");

        // Shortcut for special case, as sometimes, the very first time
        // through, the score is invalid.
        if (size() == 1) {
            const combo_tree& tr = get_tree(*begin());
            if (_visited_exemplars.find(tr) == _visited_exemplars.end())
                return begin();
        }

        // If the diversity penalty is enabled, then punish the scores
        // of those exemplars that are too similar to the previous one.
        // This typically won't make  any difference for the first dozen
        // exemplars choosen, but starts getting important once the 
        // metapopulation gets large, and the search bogs down.
        //
        // XXX The implementation here results in a lot of copying of
        // behavioral scores and combo trees, and thus could hurt
        // performance by quite a bit.  To avoid this, we'd need to
        // change the use of bscored_combo_tree_set in this class. This
        // would be a fairly big task, and it's currently not clear that
        // its worth the effort, as diversity_penalty is not yet showing
        // promising results...
        if (params.use_diversity_penalty) {
            bscored_combo_tree_set pool;
            // Behavioral score of the (previous) exemplar.
            const behavioral_score& exbs = get_bscore(_exemplar);
            for (const_iterator it = begin(); it != end(); ++it) {
                const behavioral_score &bs = get_bscore(*it);
                OC_ASSERT(bs.size(), "Behavioral score is needed for diversity!");

                score_t penalty = 1.0 / (1.0 + lp_distance(exbs, bs, 1.0));

                composite_score cs = get_composite_score(*it);
                cs.set_diversity_penalty(penalty);
                pool.insert(bscored_combo_tree(get_tree(*it),
                     composite_behavioral_score(get_pbscore(*it), cs)));
            }

            // Replace the existing metapopulation with the new one.
            swap(pool);
        }

        vector<score_t> probs;
        // Set flag to true, when a suitable exemplar is found.
        bool found_exemplar = false;
#define UNEVALUATED_SCORE -1.0e37
        score_t highest_score = UNEVALUATED_SCORE;

        // The exemplars are stored in order from best score to worst;
        // the iterator follows this order.
        for (const_iterator it = begin(); it != end(); ++it) {

            score_t sc = get_weighted_score(*it);

            // Skip any exemplars we've already used in the past.
            const combo_tree& tr = get_tree(*it);
            if (_visited_exemplars.find(tr) == _visited_exemplars.end()) {
                probs.push_back(sc);
                found_exemplar = true;
                if (highest_score < sc) highest_score = sc;
            } else // hack: if the tree is visited then put a positive
                   // score so we know it must be ignored
#define SKIP_OVER_ME (1.0e38)
                probs.push_back(SKIP_OVER_ME);
        }

        // Nothing found, we've already tried them all.
        if (!found_exemplar) {
            return end();
        }

        // Compute the probability normalization, needed for the
        // roullete choice of exemplars with equal scores, but
        // differing complexities. Empirical work on 4-parity suggests
        // that a temperature of 3 or 4 works best.
        score_t inv_temp = 100.0f / params.complexity_temperature;
        score_t sum = 0.0f;
        // Convert scores into (non-normalized) probabilities
        foreach (score_t& p, probs) {
            // In case p has the max complexity (already visited) then
            // the probability is set to null
            p = (p > (0.1*SKIP_OVER_ME) ? 0.0f : expf((p - highest_score) * inv_temp));
            sum += p;
        }

        OC_ASSERT(sum > 0.0f, "There is an internal bug, please fix it");

        size_t fwd = distance(probs.begin(), roulette_select(probs.begin(),
                                                             probs.end(),
                                                             sum, randGen()));
        // cout << "select_exemplar(): sum=" << sum << " fwd =" << fwd
        // << " size=" << probs.size() << " frac=" << fwd/((float)probs.size()) << endl;
        return std::next(begin(), fwd);
    }

    /// Given the current complexity temp, return the range of scores that
    /// are likely to be selected by the select_exemplar routine. Due to
    /// exponential decay of scores in select_exemplar(), this is fairly
    /// narrow: viz: e^30 = 1e13 ... We could probably get by with
    /// e^14 = 1.2e6
    //
    score_t useful_score_range() const
    {
        return params.complexity_temperature * 30.0 / 100.0;
    }

    /// Merge candidates in to the metapopulation.
    /// If the include-dominated flag is not set, the set of candidates
    /// might be changed during merge, with the dominated candidates
    /// removed during the merge. XXX Really?  It looks like the code
    /// does this culling *before* this method is called ...
    template<typename Candidates>
    void merge_candidates(Candidates& candidates)
    {
        // Note that merge_nondominated() is very cpu-expensive and
        // complex...
        if (params.include_dominated)
            insert(candidates.begin(), candidates.end());
        else
            // merge_nondominated_any(candidates);
            merge_nondominated(candidates, params.jobs);

        // Weed out excessively bad scores. The select_exemplar()
        // routine picks an exemplar out of the metapopulation, using
        // an exponential distribution of the score. Scores that
        // are much worse than the best scores are extremely unlikely
        // to be choosen, so discard these from the metapopulation.
        // Keeping the metapop small brings huge benefits to the
        // mem usage and runtime performance.

        // However, lets not get over-zelous; if the metapop is too small,
        // then we have the nasty situation where none of the best-scoring
        // individuals lead to a solution.  Fix the minimum metapop size
        // to, oh, say, 250.
        //
        // But if the population starts exploding, this is also bad, as
        // it chews up RAM with unlikely exemplars. Keep it in check by
        // applying more and more stringent bounds on the allowable scores.
        // The current implementation of useful_score_range() returns a
        // value a bit on the large size, by a factor of 2 or so, so its
        // quite OK to cut back on this value.

#define MIN_POOL_SIZE 250
        if (size() < MIN_POOL_SIZE) return;

        score_t top_score = get_weighted_score(*begin());
        score_t range = useful_score_range();
        score_t worst_score = top_score - range;

#if 0
        // Erase all the lowest scores.  The metapop is not kept in
        // weighted-score order, so we have to remove one at a time.
        iterator it = begin();
        while (it != end()) {
            score_t sc = get_weighted_score(*it);
            if (sc < worst_score)
                it = erase (it);
            else
                it++;
        }
#else
        // Erase all the lowest scores.  The metapop is in quasi-sorted
        // order (since the deme was sorted before being appended), so
        // this bulk remove mostly works "correctly". It is also 25%
        // faster than above. I think this is because the erase() above
        // causes the std::set to try to sort the contents, and this
        // ends up costing a lot.  I think... not sure.
        iterator it = begin();
        for (int i=0; i<MIN_POOL_SIZE; i++) it++;
        while (it != end()) {
            score_t sc = get_weighted_score(*it);
            if (sc < worst_score) break;
            it++;
        }

        erase(it, end());

        // Is the population still too large?  Yes, it is, if it is more
        // than 50 times the size of the current number of generations.
        // Realisitically, we could never explore more than 2% of a pool
        // that size.  For 10Bytes per table row, 20K rows, generation=500
        // this will still eat up tens of GBytes of RAM, and so is a
        // relatively lenient cap.
        // popsize cap =  50*(x+250)*(1+2*exp(-x/500))
        //
        // XXX TODO fix the cap so its more sensitive to the size of
        // each exemplar, right!?
        // size_t nbelts = get_bscore(*begin()).size();
        // double cap = 1.0e6 / double(nbelts);
        double cap = 50.0;
        cap *= _n_expansions + 250.0;
        cap *= 1 + 2.0*exp(- double(_n_expansions) / 500.0);
        size_t popsz_cap = cap;
        size_t popsz = size();
        while (popsz_cap < popsz)
        {
            // Leave the first 50 alone.
#define OFFSET 50
            int which = OFFSET + randGen().randint(popsz-OFFSET);
            // using std is necessary to break the ambiguity between
            // boost::next and std::next. Weirdly enough this appears
            // only 32bit arch
            erase(std::next(begin(), which));
            popsz --;
        }
#endif
    }

    /**
     * expand -- Run one deme-creation and optimization step.
     *
     * A single step consists of representation-building, to create
     * a deme, followed by optimization, (according to the specified
     * optimizer and scoring function), and finally, a merger of
     * the unique (and possibly non-dominated) trees back into the
     * metapopulation, for potential use as exemplars for futre demes.
     *
     * @param max_evals    the max evals
     *
     * @return return true if expansion has succeeded, false otherwise
     *
     */
    bool expand(int max_evals)
    {
        if (!create_deme())
            return false;

        _n_expansions ++;
        _n_evals += optimize_deme(max_evals);

        bool done = close_deme();

        if (logger().isInfoEnabled()) {
            logger().info()
               << "Expansion " << _n_expansions
               << " total number of evaluations so far: " << _n_evals;
            log_best_candidates();
        }

        // Might be empty, if the eval fails and throws an exception
        return done || empty();
    }

    /**
     * Create the deme
     *
     * @return return true if it creates deme successfully,otherwise false.
     */
    bool create_deme()
    {
        using namespace reduct;

        if (_rep != NULL || _deme != NULL)
            return false;

        if (empty())
            return false;

        // Attempt to create a non-empty representation, by looping
        // over exemplars until we find one that expands.
        do {
            const_iterator exemplar = select_exemplar();

            // Should have found something by now.
            if (exemplar == end()) {

                // XXX There is currently no way to set the revisit flag
                // using the command-line options...
                if (params.revisit) {

                    _visited_exemplars.clear();

                    logger().info(
                        "All exemplars in the metapopulation have been "
                        "visited, but it was impossible to build a "
                        "representation for any of them.  All exemplars "
                        "have been untagged and will be visited again.");

                    continue;

                } else {

                    logger().warn(
                        "WARNING: All exemplars in the metapopulation have "
                        "been visited, but it was impossible to build a "
                        "representation for any of them.  Perhaps the reduct "
                        "effort for knob building is too high.");
                    return false;
                }
            }

            _exemplar = *exemplar;

            if (logger().isDebugEnabled()) {
                logger().debug()
                    << "Attempt to build rep from exemplar: " << get_tree(_exemplar)
                    << "\nScored: " << _cscorer(get_tree(_exemplar));
            }

            // [HIGHLY EXPERIMENTAL]. It allows to select features
            // that provide the most information when combined with
            // the exemplar
            operator_set ignore_ops = params.ignore_ops;
            if (params.fstor) {
                // return the set of selected features as column index
                // (left most column corresponds to 0)
                auto selected_features = (*params.fstor)(_exemplar);
                // add the complementary of the selected features in ignore_ops
                unsigned arity = params.fstor->ctable.get_arity();
                for (unsigned i = 0; i < arity; i++)
                    if (selected_features.find(i) == selected_features.end())
                        ignore_ops.insert(argument(i + 1));
                // debug print
                // std::vector<std::string> ios;
                // auto vertex_to_str = [](const vertex& v) {
                //     std::stringstream ss;
                //     ss << v;
                //     return ss.str();
                // };
                // boost::transform(ignore_ops, back_inserter(ios), vertex_to_str);
                // printlnContainer(ios);
                // ~debug print
            }
            
            // Build a representation by adding knobs to the exemplar,
            // creating a field set, and a mapping from field set to knobs.
            _rep = new representation(*simplify_candidate,
                                      *simplify_knob_building,
                                      get_tree(_exemplar), _type_sig,
                                      ignore_ops,
                                      params.perceptions,
                                      params.actions);

            // If the representation is empty, try the next
            // best-scoring exemplar.
            if (_rep->fields().empty()) {
                delete(_rep);
                _rep = NULL;
                _visited_exemplars.insert(get_tree(_exemplar));
                // Logger
                logger().info("The representation is empty, perhaps the reduct "
                               "effort for knob building is too high");
                // ~Logger
            }
        } while (!_rep);

        // Create an empty deme.
        _deme = new deme_t(_rep->fields());

        _evals_before_this_deme = n_evals();

        return true;
    }

    /**
     * Do some optimization according to the scoring function.
     *
     * @param max_evals the max evals
     *
     * @return return the number of evaluations actually performed,
     */
    int optimize_deme(int max_evals)
    {
        if (logger().isDebugEnabled()) {
            logger().debug()
               << "Optimize deme; max evaluations allowed: "
               << max_evals;
        }

        complexity_based_scorer<CScoring> cpx_scorer =
            complexity_based_scorer<CScoring>(_cscorer, *_rep, params.reduce_all);
        return optimize(*_deme, cpx_scorer, max_evals);
    }

    /**
     * close deme
     * 1) mark the current deme exemplar to not explore it again,
     * 2) merge non-dominated candidates in the metapopulation,
     * 3) delete the deme instance from memory.
     * Return true if further deme exploration should be halted.
     */
    bool close_deme()
    {
        if (_rep == NULL || _deme == NULL)
            return false;

        size_t eval_during_this_deme = std::min(n_evals() - _evals_before_this_deme,
                                             _deme->size());

        logger().debug("Close deme; evaluations performed: %d",
                           eval_during_this_deme);

        // Mark the exemplar so we won't expand it again
        _visited_exemplars.insert(get_tree(_exemplar));

        // Add, as potential exemplars for future demes, all unique
        // trees in the final deme.
        metapop_candidates pot_candidates;

        logger().debug("Sort the deme");

        // Sort the deme according to composite_score (descending order)
        std::sort(_deme->begin(), _deme->end(),
                  std::greater<scored_instance<composite_score> >());

        // Trim the deme down to size.  The point here is that the next
        // stage, select_candidates below, is very cpu-intensive; we
        // should keep only those candidates that will survive in the
        // metapop.  But what are these? Well, select_exemplar() uses
        // an exponential choice function; instances below a cut-off
        // score have no chance at all of getting selected. So just
        // eliminate them now, instead of later.
        //
        // However, trimming too much is bad: it can happen that none
        // of the best-scoring instances lead to a solution. So keep
        // around a reasonable pool. Wild choice ot 250 seems reasonable.
        if (MIN_POOL_SIZE < _deme->size()) {
            score_t top_sc = get_weighted_score(_deme->begin()->second);
            score_t bot_sc = top_sc - useful_score_range();

            for (size_t i = _deme->size()-1; 0 < i; --i) {
                const composite_score &cscore = (*_deme)[i].second;
                score_t score = get_weighted_score(cscore);
                if (score < bot_sc) {
                    _deme->pop_back();
                }
            }

            eval_during_this_deme =
                std::min(eval_during_this_deme, _deme->size());
        }

        ///////////////////////////////////////////////////////////////
        // select the set of candidates to add in the metapopulation //
        ///////////////////////////////////////////////////////////////
        logger().debug("Select candidates to merge");

        typedef boost::shared_mutex mutex;
        typedef boost::shared_lock<mutex> shared_lock;
        typedef boost::unique_lock<mutex> unique_lock;

        mutex pot_cnd_mutex; // mutex for pot_candidates

        // NB, this is an anonymous function. In particular, some
        // compilers require that members be explicitly referenced
        // with this-> as otherwise we get compile fails:
        // https://bugs.launchpad.net/bugs/933906
        auto select_candidates =
            [&, this](const scored_instance<composite_score>& inst)
        {
            const composite_score& inst_csc = inst.second;
            score_t inst_sc = get_score(inst_csc);

            // if it's really bad stops
            if (inst_sc <= worst_score || !isfinite(inst_sc))
                return;

            int pot_candidates_size = [&]() {
                shared_lock lock(pot_cnd_mutex);
                return pot_candidates.size(); }();

            // only add up to max_candidates
            if (this->params.max_candidates < 0
                || pot_candidates_size < this->params.max_candidates) {

                // Get the combo_tree associated to inst, cleaned and reduced.
                //
                // @todo: below, the candidate is reduced possibly for the
                // second time.  This second reduction could probably be
                // avoided with some clever cache or something. (or a flag?)
                combo_tree tr = this->_rep->get_candidate(inst, true);

                // Look for tr in the list of potential candidates.
                // Return true if not found.
                auto thread_safe_tr_not_found = [&]() {
                    shared_lock lock(pot_cnd_mutex);
                    return pot_candidates.find(tr) == pot_candidates.end();
                };

                bool not_already_visited = this->_visited_exemplars.find(tr)
                    == this->_visited_exemplars.end();

                // update the set of potential exemplars
                if (not_already_visited && thread_safe_tr_not_found()) {
                    penalized_behavioral_score pbs; // empty bscore till it gets computed
                    composite_behavioral_score cbsc(pbs, inst_csc);
                    {
                        unique_lock lock(pot_cnd_mutex);
                        pot_candidates[tr] = cbsc;
                    }
                }
            }
        };

        // We use deme->begin() + eval_during_this_deme instead of
        // deme->end(), because we might have resized the deme to
        // something larger than the actual number of instances we
        // placed into it (XXX really? Does this ever happen?)
        //
        // Note: this step can be very time consuming; it currently
        // takes anywhere from 25 to 500(!!) millisecs per instance (!!)
        // for me; my (reduced, simplified) instances have complexity
        // of about 100. This seems too long/slow.
        deme_cit deme_begin = _deme->begin();
        deme_cit deme_end = _deme->begin() + eval_during_this_deme;
        OMP_ALGO::for_each(deme_begin, deme_end, select_candidates);

        // Behavioural scores are needed only if domination-based
        // merging is asked for, or if the diversity penalty is in use.
        //  Save CPU time by not computing them.
        if (!params.include_dominated || params.use_diversity_penalty) {
            logger().debug("Compute behavioral score of %d selected candidates",
                           pot_candidates.size());

            auto compute_bscore = [this](metapop_candidates::value_type& cand) {
                composite_score csc = get_composite_score(cand.second);
                penalized_behavioral_score pbs = this->_bscorer(cand.first);
                cand.second = composite_behavioral_score(pbs, csc);
            };
            OMP_ALGO::for_each(pot_candidates.begin(), pot_candidates.end(),
                               compute_bscore);
        }

        bscored_combo_tree_set candidates = get_new_candidates(pot_candidates);
        if (!params.include_dominated) {

            logger().debug("Remove dominated candidates");
            if (logger().isFineEnabled()) {
                logger().fine("Candidates with their bscores before"
                              " removing the dominated candidates");
                stringstream ss;
                logger().fine(ostream(ss, candidates.begin(), candidates.end(),
                                      -1, true, true, true).str());
            }

            size_t old_size = candidates.size();
            remove_dominated(candidates, params.jobs);

            logger().debug("Removed %u dominated candidates out of %u",
                           old_size - candidates.size(), old_size);
            if (logger().isFineEnabled()) {
                logger().fine("Candidates with their bscores after"
                              " removing the dominated candidates");
                stringstream ss;
                logger().fine(ostream(ss, candidates.begin(), candidates.end(),
                                      -1, true, true, true).str());
            }
        }

        // update the record of the best-seen score & trees
        update_best_candidates(candidates);

        if (logger().isDebugEnabled()) {
            logger().debug("Merge %u candidates with the metapopulation",
                           candidates.size());
            if (logger().isFineEnabled()) {
                stringstream ss;
                ss << "Candidates to merge with the metapopulation:" << std::endl;
                logger().fine(ostream(ss, candidates.begin(), candidates.end(),
                                      -1, true, true).str());
            }
        }

        bool done = false;
        if (params.merge_callback)
            done = (*params.merge_callback)(candidates, params.callback_user_data);
        merge_candidates(candidates);

        if (logger().isDebugEnabled()) {
            logger().debug("Metapopulation size is %u", size());
            if (logger().isFineEnabled()) {
                stringstream ss;
                ss << "Metapopulation after merging:" << std::endl;
                logger().fine(ostream(ss, -1, true, true).str());
            }
        }

        delete _deme;
        delete _rep;
        _deme = NULL;
        _rep = NULL;

        return done;
    }

    // Return the set of candidates not present in the metapopulation.
    // This makes merging faster because it decreases the number of
    // calls of dominates.
    bscored_combo_tree_set get_new_candidates(const metapop_candidates& mcs)
    {
        bscored_combo_tree_set res;
        foreach (bscored_combo_tree cnd, mcs)
            if (find(cnd) == end())
                res.insert(cnd);
        return res;
    }

    typedef pair<bscored_combo_tree_set,
                 bscored_combo_tree_set> bscored_combo_tree_set_pair;

    typedef std::vector<const bscored_combo_tree*> bscored_combo_tree_ptr_vec;
    typedef bscored_combo_tree_ptr_vec::iterator bscored_combo_tree_ptr_vec_it;
    typedef bscored_combo_tree_ptr_vec::const_iterator bscored_combo_tree_ptr_vec_cit;
    typedef pair<bscored_combo_tree_ptr_vec,
                 bscored_combo_tree_ptr_vec> bscored_combo_tree_ptr_vec_pair;
    typedef std::set<const bscored_combo_tree*> bscored_combo_tree_ptr_set;

    // reciprocal of random_access_view
    static bscored_combo_tree_set
    to_set(const bscored_combo_tree_ptr_vec& bcv) {
        bscored_combo_tree_set res;
        foreach(const bscored_combo_tree* cnd, bcv)
            res.insert(*cnd);
        return res;
    }

    void remove_dominated(bscored_combo_tree_set& bcs, unsigned jobs = 1)
    {
        // get the nondominated candidates
        bscored_combo_tree_ptr_vec bcv = random_access_view(bcs);
        bscored_combo_tree_ptr_vec res = get_nondominated_rec(bcv, jobs);
        // get the dominated by set difference
        boost::sort(bcv); boost::sort(res);
        bscored_combo_tree_ptr_vec dif = set_difference(bcv, res);
        // remove the dominated ones
        foreach(const bscored_combo_tree* cnd_ptr, dif)
            bcs.erase(*cnd_ptr);
    }

    static bscored_combo_tree_set
    get_nondominated_iter(const bscored_combo_tree_set& bcs)
    {
        typedef std::list<bscored_combo_tree> bscored_combo_tree_list;
        typedef bscored_combo_tree_list::iterator bscored_combo_tree_list_it;
        bscored_combo_tree_list mcl(bcs.begin(), bcs.end());
        // remove all dominated candidates from the list
        for(bscored_combo_tree_list_it it1 = mcl.begin(); it1 != mcl.end();) {
            bscored_combo_tree_list_it it2 = it1;
            ++it2;
            if(it2 != mcl.end())
                for(; it2 != mcl.end();) {
                    tribool dom = dominates(it1->second, it2->second);
                    if(dom)
                        it2 = mcl.erase(it2);
                    else if(!dom) {
                        it1 = mcl.erase(it1);
                        it2 = mcl.end();
                    } else
                        ++it2;
                    if(it2 == mcl.end())
                        ++it1;
                }
            else
                ++it1;
        }
        return bscored_combo_tree_set(mcl.begin(), mcl.end());
    }

    // split in 2 of equal size
    static bscored_combo_tree_ptr_vec_pair
    inline split(const bscored_combo_tree_ptr_vec& bcv)
    {
        bscored_combo_tree_ptr_vec_cit middle = bcv.begin() + bcv.size() / 2;
        return make_pair(bscored_combo_tree_ptr_vec(bcv.begin(), middle),
                         bscored_combo_tree_ptr_vec(middle, bcv.end()));
    }

    bscored_combo_tree_ptr_vec
    get_nondominated_rec(const bscored_combo_tree_ptr_vec& bcv,
                         unsigned jobs = 1)
    {
        ///////////////
        // base case //
        ///////////////
        if (bcv.size() < 2) {
            return bcv;
        }
        //////////////
        // rec case //
        //////////////
//  The names in enum std::launch have not yet been standardized.
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ >= 5) && (__GNUC_MINOR__ < 7)
 #define LAUNCH_SYNC std::launch::sync
#else
 #define LAUNCH_SYNC std::launch::deferred
#endif
        bscored_combo_tree_ptr_vec_pair bcv_p = split(bcv);
        if (jobs > 1) { // multi-threaded
            auto s_jobs = split_jobs(jobs); // pair
            // recursive calls
            std::future<bscored_combo_tree_ptr_vec> task =
                std::async(jobs > 1 ? std::launch::async : LAUNCH_SYNC,
                           bind(&self::get_nondominated_rec, this,
                                bcv_p.first, s_jobs.first));
            bscored_combo_tree_ptr_vec bcv2_nd =
                get_nondominated_rec(bcv_p.second, s_jobs.second);
            bscored_combo_tree_ptr_vec_pair res_p =
                get_nondominated_disjoint_rec(task.get(), bcv2_nd, jobs);
            // union and return
            append(res_p.first, res_p.second);
            return res_p.first;
        } else { // single-threaded
            // recursive calls
            bscored_combo_tree_ptr_vec
                bcv1_nd = get_nondominated_rec(bcv_p.first),
                bcv2_nd = get_nondominated_rec(bcv_p.second);
            bscored_combo_tree_ptr_vec_pair
                res_p = get_nondominated_disjoint_rec(bcv1_nd, bcv2_nd);
            // union and return
            append(res_p.first, res_p.second);
            return res_p.first;
        }
    }

    // return a pair of sets of nondominated candidates between bcs1
    // and bcs2, assuming none contain dominated candidates. Contrary
    // to what the name of function says, the 2 sets do not need be
    // disjoint, however there are inded disjoint according to the way
    // they are used in the code. The first (resp. second) element of
    // the pair corresponds to the nondominated candidates of bcs1
    // (resp. bcs2)
    bscored_combo_tree_set_pair
    get_nondominated_disjoint(const bscored_combo_tree_set& bcs1,
                              const bscored_combo_tree_set& bcs2,
                              unsigned jobs = 1)
    {
        bscored_combo_tree_ptr_vec_pair res_p =
            get_nondominated_disjoint_rec(random_access_view(bcs1),
                                          random_access_view(bcs2),
                                          jobs);
        return make_pair(to_set(res_p.first), to_set(res_p.second));
    }

    bscored_combo_tree_ptr_vec_pair
    get_nondominated_disjoint_rec(const bscored_combo_tree_ptr_vec& bcv1,
                                  const bscored_combo_tree_ptr_vec& bcv2,
                                  unsigned jobs = 1)
    {
        ///////////////
        // base case //
        ///////////////
        if (bcv1.empty() || bcv2.empty())
            return make_pair(bcv1, bcv2);
        else if (bcv1.size() == 1) {
            bscored_combo_tree_ptr_vec bcv_res1, bcv_res2;
            bscored_combo_tree_ptr_vec_cit it1 = bcv1.begin(),
                it2 = bcv2.begin();
            bool it1_insert = true; // whether *it1 is to be inserted
                                    // in bcv_res1
            for (; it2 != bcv2.end(); ++it2) {
                tribool dom = dominates(get_pbscore(**it1).first, get_pbscore(**it2).first);
                if (!dom) {
                    it1_insert = false;
                    bcv_res2.insert(bcv_res2.end(), it2, bcv2.end());
                    break;
                } else if (indeterminate(dom))
                    bcv_res2.push_back(*it2);
            }
            if (it1_insert)
                bcv_res1.push_back(*it1);
            return make_pair(bcv_res1, bcv_res2);
        }
        //////////////
        // rec case //
        //////////////
        // split bcs1 in 2
        bscored_combo_tree_ptr_vec_pair bcv1_p = split(bcv1);
        if(jobs > 1) { // multi-threaded
            unsigned jobs1 = jobs / 2;
            unsigned jobs2 = std::max(1U, jobs - jobs1);
            std::future<bscored_combo_tree_ptr_vec_pair> task =
                std::async(std::launch::async,
                           bind(&self::get_nondominated_disjoint_rec, this,
                                bcv1_p.first, bcv2, jobs1));
            bscored_combo_tree_ptr_vec_pair bcv_m2 =
                get_nondominated_disjoint_rec(bcv1_p.second, bcv2, jobs2);
            bscored_combo_tree_ptr_vec_pair bcv_m1 = task.get();
            // merge results
            append(bcv_m1.first, bcv_m2.first);
            boost::sort(bcv_m1.second); boost::sort(bcv_m2.second);
            bscored_combo_tree_ptr_vec bcv_m2_inter =
                set_intersection(bcv_m1.second, bcv_m2.second);
            return make_pair(bcv_m1.first, bcv_m2_inter);
        } else { // single-threaded
            bscored_combo_tree_ptr_vec_pair
                bcv_m1 = get_nondominated_disjoint_rec(bcv1_p.first, bcv2),
                bcv_m2 = get_nondominated_disjoint_rec(bcv1_p.second,
                                                       bcv_m1.second);
            // merge results
            append(bcv_m1.first, bcv_m2.first);
            return make_pair(bcv_m1.first, bcv_m2.second);
        }
    }

    // merge nondominated candidate to the metapopulation assuming
    // that bcs contains no dominated candidates within itself
    void merge_nondominated(bscored_combo_tree_set& bcs, unsigned jobs = 1)
    {
        bscored_combo_tree_ptr_vec bcv_mp = random_access_view(*this),
            bcv = random_access_view(bcs);
        bscored_combo_tree_ptr_vec_pair bcv_p =
            get_nondominated_disjoint_rec(bcv, bcv_mp, jobs);
        // remove the nondominates ones from the metapopulation
        boost::sort(bcv_mp);
        boost::sort(bcv_p.second);
        bscored_combo_tree_ptr_vec diff_bcv_mp =
            set_difference(bcv_mp, bcv_p.second);

        foreach (const bscored_combo_tree* cnd, diff_bcv_mp)
            erase(*cnd);

        // add the non dominates ones from bsc
        foreach (const bscored_combo_tree* cnd, bcv_p.first)
            insert(*cnd);
    }

    // Iterative version of merge_nondominated
    void merge_nondominated_iter(bscored_combo_tree_set& bcs)
    {
        for (bscored_combo_tree_set_it it1 = bcs.begin(); it1 != bcs.end();) {
            bscored_combo_tree_set_it it2 = begin();
            if (it2 == end())
                break;
            for (; it2 != end();) {
                tribool dom = dominates(it1->second, it2->second);
                if (dom)
                    erase(it2++);
                else if (!dom) {
                    bcs.erase(it1++);
                    it2 = end();
                } else
                    ++it2;
                if (it2 == end())
                    ++it1;
            }
        }
        // insert the nondominated candidates from bcs
        insert (bcs.begin(), bcs.end());
    }

    // like merge_nondominated_iter but doesn't make any assumption on
    // bcs
    void merge_nondominated_any(const bscored_combo_tree_set& bcs)
    {
        bscored_combo_tree_set_cit from = bcs.begin(), to = bcs.end();
        for (;from != to;++from) {
            bool nondominated = true;
            for (iterator it = begin(); it != end();) {
                tribool dom = dominates(from->second, it->second);
                if (dom) {
                    erase(it++);
                } else if (!dom) {
                    nondominated = false;
                    break;
                } else {
                    ++it;
                }
            }
            if (nondominated)
                insert(*from);
        }
    }

    /**
     * return true if x dominates y
     *        false if y dominates x
     *        indeterminate otherwise
     */
    static inline tribool dominates(const behavioral_score& x,
                                    const behavioral_score& y)
    {
        // everything dominates an empty vector
        if (x.empty()) {
            if (y.empty())
                return indeterminate;
            return false;
        } else if (y.empty()) {
            return true;
        }

        tribool res = indeterminate;
        for (behavioral_score::const_iterator xit = x.begin(), yit = y.begin();
             xit != x.end();++xit, ++yit)
        {
            if (*xit > *yit) {
                if (!res)
                    return indeterminate;
                else
                    res = true;
            } else if (*yit > *xit) {
                if (res)
                    return indeterminate;
                else
                    res = false;
            }
        }
        return res;
    }

    // Update the record of the best score seen, and the associated tree.
    void update_best_candidates(const bscored_combo_tree_set& candidates)
    {
        if (!candidates.empty()) {

            // Candidates are kept in weighted score order, not in
            // absolute score order.  Thus, we need to search through
            // the first few to find the true best score.  Also, there
            // may be several candidates with the best score.
            score_t best_score = get_score(_best_cscore);
            complexity_t best_cpx = get_complexity(_best_cscore);

            foreach(const bscored_combo_tree& it, candidates)
            {
                const composite_score& cit = get_composite_score(it);
                score_t sc = get_score(cit);
                complexity_t cpx = get_complexity(cit);
                if ((sc > best_score) ||
                    ((sc == best_score) && (cpx <= best_cpx)))
                {
                    if ((sc > best_score) ||
                        ((sc == best_score) && (cpx < best_cpx)))
                    {
                        _best_cscore = cit;
                        best_score = get_score(_best_cscore);
                        best_cpx = get_complexity(_best_cscore);
                        _best_candidates.clear();
                        logger().debug() << "New best score: " << _best_cscore;
                    }
                    _best_candidates.insert(it);
                }
            }
        }
    }

    // log the best candidates
    void log_best_candidates() const
    {
        if (!logger().isInfoEnabled())
            return;

        if (best_candidates().empty())
            logger().info("No new best candidates");
        else {
            logger().info()
               << "The following candidate(s) have the best score "
               << best_composite_score();
            foreach(const bscored_combo_tree& cand, best_candidates()) {
                logger().info() << "" << get_tree(cand);
            }
        }
    }

    /**
     * stream out the metapopulation in decreasing order of their
     * score along with their scores (optionally complexity and
     * bscore).  If n is negative, then stream them all out.  Note
     * that the default sort order for the metapop is a weighted
     * linear combo of the best scores and the smallest complexities,
     * so that the best-ranked candidates are not necessarily those
     * with the best raw score.
     */
    template<typename Out, typename In>
    Out& ostream(Out& out, In from, In to, long n = -1,
                 bool output_score = true,
                 bool output_complexity = false,
                 bool output_bscore = false,
                 bool output_only_bests = false,
                 bool output_python = false)
    {
        if (!output_only_bests) {
            for (; from != to && n != 0; ++from, n--) {
                ostream_bscored_combo_tree(out, *from, output_score,
                                           output_complexity, output_bscore, output_python);
            }
            return out;
        }

        // Else, search for the top score...
        score_t best_score = worst_score;

        for (In f = from; f != to; ++f) {
            const bscored_combo_tree& bt = *f;
            score_t sc = get_score(bt);
            if (best_score < sc) best_score = sc;
        }

        // And print only the top scorers.
        // The problem here is that the highest scorers are not
        // necessarily ranked highest, as the ranking is a linear combo
        // of both score and complexity.
        for (In f = from; f != to && n != 0; ++f, n--) {
            const bscored_combo_tree& bt = *f;
            if (best_score <= get_score(bt)) {
                ostream_bscored_combo_tree(out, *f, output_score,
                                           output_complexity, output_bscore, output_python);
            }
        }
        return out;
    }

    // Like above, but assumes that from = begin() and to = end().
    template<typename Out>
    Out& ostream(Out& out, long n = -1,
                 bool output_score = true,
                 bool output_complexity = false,
                 bool output_bscore = false,
                 bool output_only_bests = false,
                 bool output_python = false)
    {
        return ostream(out, begin(), end(),
                       n, output_score, output_complexity,
                       output_bscore, output_only_bests, output_python);
    }

    ///  hmmm, apparently it's ambiguous with the one below, strange
    // // Like above but assumes that from = c.begin() and to = c.end()
    // template<typename Out, typename C>
    // Out& ostream(Out& out, const C& c, long n = -1,
    //              bool output_score = true,
    //              bool output_complexity = false,
    //              bool output_bscore = false,
    //              bool output_dominated = false)
    // {
    //     return ostream(out, c.begin(), c.end(),
    //                    n, output_score, output_complexity,
    //                    output_bscore, output_dominated);
    // }

    // Like above, but using std::cout.
    void print(long n = -1,
               bool output_score = true,
               bool output_complexity = false,
               bool output_bscore = false,
               bool output_only_bests = false)
    {
        ostream(std::cout, n, output_score, output_complexity,
                output_bscore, output_only_bests);
    }

    const combo::type_tree& _type_sig;    // type signature of the exemplar
    const reduct::rule* simplify_candidate; // to simplify candidates
    const reduct::rule* simplify_knob_building; // during knob building
    const CScoring& _cscorer; // composite score
    const BScoring& _bscorer; // behavioral score
    Optimization &optimize;
    metapop_parameters params;

protected:
    size_t _n_evals;
    size_t _n_expansions;
    size_t _evals_before_this_deme;

    // The best score ever found during search.
    composite_score _best_cscore;

    // Trees with composite score equal to _best_cscore.
    metapop_candidates _best_candidates;

    // contains the exemplars of demes that have been searched so far
    combo_tree_hash_set _visited_exemplars;

    representation* _rep; // representation of the current deme
    typedef instance_set<composite_score> deme_t;
    typedef deme_t::iterator deme_it;
    typedef deme_t::const_iterator deme_cit;
    deme_t* _deme; // current deme

    // exemplar of the current deme; a copy, not a reference.
    bscored_combo_tree _exemplar;
};

} // ~namespace moses
} // ~namespace opencog

#endif // _OPENCOG_METAPOPULATION_H
