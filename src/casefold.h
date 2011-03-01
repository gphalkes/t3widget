/* Copyright (C) 2010 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CASEFOLD_H
#define CASEFOLD_H

#include <string>
#include <vector>
using namespace std;

class line_t;

class case_fold_t {
	private:
		struct replacement_t {
			char base[4];
			string replacement;

			replacement_t(int _base);
			void append(int _replacement);
			bool operator<(const replacement_t &other) const;
			bool operator<(const char *other) const;
		};

		typedef vector<replacement_t> replacements_t;

		static replacements_t replacements;
	public:
		static int init(const char *fileName);
		static line_t *fold(const line_t *line, int *pos);
		static string *fold(const string *line, string *result = NULL);
};

#endif
