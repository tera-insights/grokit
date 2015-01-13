//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
/*-------------------------------------------------------------------------
 *
 * like_match.c
 *	  like expression handling internal code.
 *
 * Copyright (c) 1996-2006, PostgreSQL Global Development Group
 *-------------------------------------------------------------------------
 */

/*
**	Originally written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986.
**	Rich $alz is now <rsalz@bbn.com>.
**	Special thanks to Lars Mathiesen <thorinn@diku.dk> for the LABORT code.
**
**	This code was shamelessly stolen from the "pql" code by myself and
**	slightly modified :)
**
**	All the nice shell RE matching stuff was replaced by just "_" and "%"
**
**	As I don't have a copy of the SQL standard handy I wasn't sure whether
**	to leave in the '\' escape character handling.
**
**	Keith Parks. <keith@mtcc.demon.co.uk>
**
**	SQL92 lets you specify the escape character by saying
**	LIKE <pattern> ESCAPE <escape character>. We are a small operation
**	so we force you to use '\'. - ay 7/95
*/

#define LIKE_TRUE						true
#define LIKE_FALSE					false
#define LIKE_ABORT					false

/* Set up to compile like_match.c for single-byte characters */
#define CHAREQ(p1, p2) (*(p1) == *(p2))
#define ICHAREQ(p1, p2) (tolower((unsigned char) *(p1)) == tolower((unsigned char) *(p2)))
//#define NextChar(p, plen) ((p)++, (plen)--)
#define NextChar(p) ((p)++)
#define CopyAdvChar(dst, src, srclen) (*(dst)++ = *(src)++, (srclen)--)

#include <iostream>

using namespace std;

/*--------------------
 *	Match text and p, return LIKE_TRUE, LIKE_FALSE, or LIKE_ABORT.
 *
 *	LIKE_TRUE: they match
 *	LIKE_FALSE: they don't match
 *	LIKE_ABORT: not only don't they match, but the text is too short.
 *
 * If LIKE_ABORT is returned, then no suffix of the text can match the
 * pattern either, so an upper-level % scan can stop scanning now.
 *--------------------
 */
bool LikeBody(const char *t, const char *p)
{
	// fastpaths moved into inline header

	while ((*t != '\0') && (*p != '\0'))
	{
		if (*p == '\\')
		{
			/* Next pattern char must match literally, whatever it is */
			NextChar(p);
			if ((*p == '\0') || !CHAREQ(t, p))
				return LIKE_FALSE;
		}
		else if (*p == '%')
		{
			/* %% is the same as % according to the SQL standard */
			/* Advance past all %'s */
			while ((*p != '\0') && (*p == '%'))
				NextChar(p);
			/* Trailing percent matches everything. */
			if (*p == '\0')
				return LIKE_TRUE;

			/*
			 * Otherwise, scan for a text position at which we can match the
			 * rest of the pattern.
			 */
			while (*t != '\0')
			{
				/*
				 * Optimization to prevent most recursion: don't recurse
				 * unless first pattern char might match this text char.
				 */
				if (CHAREQ(t, p) || (*p == '\\') || (*p == '_'))
				{
					int			matched = LikeBody(t, p);

					if (matched != LIKE_FALSE)
						return matched; /* TRUE or ABORT */
				}

				NextChar(t);
			}

			/*
			 * End of text with no match, so no point in trying later places
			 * to start matching this pattern.
			 */
			return LIKE_ABORT;
		}
		else if ((*p != '_') && !CHAREQ(t, p))
		{
			/*
			 * Not the single-character wildcard and no explicit match? Then
			 * time to quit...
			 */
			return LIKE_FALSE;
		}

		NextChar(t);
		NextChar(p);
	}

	if (*t != '\0')
		return LIKE_FALSE;		/* end of pattern, but not of text */

	/* End of input string.  Do we have matching pattern remaining? */
	while ((*p != '\0') && (*p == '%'))	/* allow multiple %'s at end of
										 * pattern */
		NextChar(p);
	if (*p == '\0')
		return LIKE_TRUE;

	/*
	 * End of text with no match, so no point in trying later places to start
	 * matching this pattern.
	 */
	return LIKE_ABORT;
}	/* MatchText() */
