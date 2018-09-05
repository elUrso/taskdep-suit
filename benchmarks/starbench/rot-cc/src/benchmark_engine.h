/*
 * Copyright (C) 2013 Michael Andersch <michael.andersch@mailbox.tu-berlin.de>
 *
 * This file is part of Starbench.
 *
 * Starbench is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Starbench is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Starbench.  If not, see <http://www.gnu.org/licenses/>.
 */


/**********************************************************************************
                INCLUDES & DEFINES
***********************************************************************************/
#ifndef B_ENGINE_H
#define B_ENGINE_H

#include "rotation_engine.h"
#include "convert_engine.h"
#include <string>

using namespace std;

class BenchmarkEngine {
public:
    bool init(string srcname, string destname, unsigned int angle);
    void run();
    void run_seq();
    void finish();
private:
    RotateEngine* re;
    ConvertEngine* ce;
};

#endif // B_ENGINE_H
