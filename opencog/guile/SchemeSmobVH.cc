/*
 * SchemeSmobVH.c
 *
 * Scheme small objects (SMOBS) for version handles.
 *
 * Copyright (c) 2008,2009 Linas Vepstas <linas@linas.org>
 */

#ifdef HAVE_GUILE

#include <libguile.h>

#include <opencog/atomspace/VersionHandle.h>
#include <opencog/guile/SchemeSmob.h>

using namespace opencog;

/* ============================================================== */

#if 0
/**
 * Search for a truth value in a list of values.
 * Return the truth value if found, else return null.
 * Throw errors if the list is not stictly just key-value pairs
 */
TruthValue * SchemeSmob::get_tv_from_list(SCM slist)
{
	while (scm_is_pair(slist))
	{
		SCM sval = SCM_CAR(slist);
		if (SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, sval))
		{
			scm_t_bits misctype = SCM_SMOB_FLAGS(sval);
			switch (misctype)
			{
				case COG_TV:
					return (TruthValue *) SCM_SMOB_DATA(sval);
				default:
					break;
			}
		}

		slist = SCM_CDR(slist);
	}

	return NULL;
}

#endif

/* ============================================================== */

std::string SchemeSmob::vh_to_string(const VersionHandle *vh)
{
#define BUFLEN 120
	char buff[BUFLEN];

	std::string ret = "(vh \"";
	ret += VersionHandle::indicatorToStr(vh->indicator);
	snprintf(buff, BUFLEN, "\" %lu)", vh->substantive.value());
	ret += buff;
	return ret;
}

/* ============================================================== */
/**
 * Take over memory management of a version handle.
 */
SCM SchemeSmob::take_vh (VersionHandle *vh)
{
	scm_gc_register_collectable_memory (vh,
	                 sizeof(*vh), "opencog vh");

	SCM smob;
	SCM_NEWSMOB (smob, cog_misc_tag, vh);
	SCM_SET_SMOB_FLAGS(smob, COG_VH);
	return smob;
}

/* ============================================================== */
/**
 * Create a new version handle
 */
SCM SchemeSmob::ss_new_vh (SCM sind, SCM shandle)
{
	Handle h = verify_handle(shandle, "cog-new-vh");
	std::string ind_name = decode_string (sind, "cog-new-vh",
		"indicator for the version handle");

	IndicatorType ind;
	try
	{
		ind = VersionHandle::strToIndicator(ind_name.c_str());
	}
	catch (InvalidParamException e)
	{
		scm_wrong_type_arg_msg("cog-new-vh", 1, sind, 
			"version handle indicator string name");
	}
	
	VersionHandle *vh = new VersionHandle(ind, h);
	return take_vh(vh);
}

/* ============================================================== */
/**
 * Return true if the scm is a version handle
 */
SCM SchemeSmob::ss_vh_p (SCM s)
{
	if (SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, s))
	{
		scm_t_bits misctype = SCM_SMOB_FLAGS(s);
		switch (misctype)
		{
			case COG_VH:
				return SCM_BOOL_T;

			default:
				return SCM_BOOL_F;
		}
	}
	return SCM_BOOL_F;
}

/* ============================================================== */
/**
 * Return association list for the version handle.
 */
SCM SchemeSmob::ss_vh_get_value (SCM s)
{
	if (!SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, s))
	{
		return SCM_EOL;
	}

	scm_t_bits misctype = SCM_SMOB_FLAGS(s);
	if (COG_VH != misctype) return SCM_EOL;

	// Return association list
	VersionHandle *vh;
	vh = (VersionHandle *) SCM_SMOB_DATA(s);

	const char * str = VersionHandle::indicatorToStr(vh->indicator);
	SCM ind = scm_from_locale_string(str);
	SCM shandle = scm_from_ulong(vh->substantive.value());
	SCM h;
	SCM_NEWSMOB (h, cog_handle_tag, shandle);
	SCM sind = scm_from_locale_symbol("indicator");
	SCM satom = scm_from_locale_symbol("atom");
			
	return scm_cons2(
		scm_cons(sind, ind),
		scm_cons(satom, h), 
		SCM_EOL);
}

#endif /* HAVE_GUILE */
/* ===================== END OF FILE ============================ */
