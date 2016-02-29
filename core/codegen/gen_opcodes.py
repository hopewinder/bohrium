#!/usr/bin/env python
import json
import sys
import os
import re
import time
import stat
import collections

"""
    Generates the include/bh_opcode.h and core/bh_opcode
    based on the definitnion in /core/codegen/opcodes.json.
"""

def gen_headerfile( opcodes ):

    enums = ("        %s = %s,\t\t// %s" % (opcode['opcode'], opcode['id'], opcode['doc']) for opcode in opcodes)
    stamp = time.strftime("%d/%m/%Y")

    l = [int(o['id']) for o in opcodes]
    l = [x for x in l if l.count(x) > 1]
    if len(l) > 0:
        raise ValueError("opcodes.json contains id duplicates: %s"%str(l))

    l = [o['opcode'] for o in opcodes]
    l = [x for x in l if l.count(x) > 1]
    if len(l) > 0:
        raise ValueError("opcodes.json contains opcode duplicates: %s"%str(l))

    max_ops = max([int(o['id']) for o in opcodes])
    return """
/*
 * Do not edit this file. It has been auto generate by
 * ../core/codegen/gen_opcodes.py at __TIMESTAMP__.
 */

#ifndef __BH_OPCODE_H
#define __BH_OPCODE_H

#include "bh_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Codes for known oparations */
enum /* bh_opcode */
{
__OPCODES__

    BH_NO_OPCODES = __NO_OPCODES__, // The amount of opcodes
    BH_MAX_OPCODE_ID = __MAX_OP__   // The extension method offset
};

/* Number of operands for operation
 *
 * @opcode Opcode for operation
 * @return Number of operands
 */
int bh_noperands(bh_opcode opcode);

#ifdef __cplusplus
}
#endif

#endif
""".replace('__TIMESTAMP__', stamp).replace('__OPCODES__', '\n'.join(enums)).replace('__NO_OPCODES__', str(len(opcodes))).replace('__MAX_OP__',str(max_ops))

def gen_cfile(opcodes):

    text    = ['        case %s: return "%s";' % (opcode['opcode'], opcode['opcode']) for opcode in opcodes]
    nops    = ['        case %s: return %s;' % (opcode['opcode'], opcode['nop']) for opcode in opcodes]
    sys_op    = ['        case %s: '%opcode['opcode'] for opcode in opcodes if opcode['system_opcode']]
    elem_op   = ['        case %s: '%opcode['opcode'] for opcode in opcodes if opcode['elementwise']]
    reduce_op = ['        case %s: '%opcode['opcode'] for opcode in opcodes if opcode['reduction']]
    accum_op  = ['        case %s: '%opcode['opcode'] for opcode in opcodes if opcode['accumulate']]
    stamp   = time.strftime("%d/%m/%Y")

    return """
/*
 * Do not edit this file. It has been auto generate by
 * ../core/codegen/gen_opcodes.py at __TIMESTAMP__.
 */

#include <stdlib.h>
#include <stdio.h>
#include <bh_opcode.h>
#include <bh.h>
#include <stdbool.h>

/* Number of operands for operation
 *
 * @opcode Opcode for operation
 * @return Number of operands
 */
int bh_noperands(bh_opcode opcode)
{
    switch(opcode)
    {
__NOPS__

    default:
        return 3;//Extension methods have 3 operands always
    }
}

/* Number of operands in instruction
 * NB: this function handles user-defined function correctly
 * @inst Instruction
 * @return Number of operands
 */
int bh_operands_in_instruction(const bh_instruction *inst)
{
    return bh_noperands(inst->opcode);
}

/* Text descriptions for a given operation */
const char* _opcode_text[BH_NONE+1];
bool _opcode_text_initialized = false;

/* Text string for operation
 *
 * @opcode Opcode for operation
 * @return Text string.
 */
const char* bh_opcode_text(bh_opcode opcode)
{
    switch(opcode)
    {
__TEXT__

        default: return "Unknown opcode";
    }
}

/* Determines if the operation is a system operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
bool bh_opcode_is_system(bh_opcode opcode)
{
    switch(opcode)
    {
__SYS_OP__
            return true;

        default:
            return false;
    }
}

/* Determines if the operation is an elementwise operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
bool bh_opcode_is_elementwise(bh_opcode opcode)
{
    switch(opcode)
    {
__ELEM_OP__
            return true;

        default:
            return false;
    }
}

/* Determines if the operation is a reduction operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
bool bh_opcode_is_reduction(bh_opcode opcode)
{
    switch(opcode)
    {
__REDUCE_OP__
            return true;

        default:
            return false;
    }
}

/* Determines if the operation is an accumulate operation
 *
 * @opcode The operation opcode
 * @return The boolean answer
 */
bool bh_opcode_is_accumulate(bh_opcode opcode)
{
    switch(opcode)
    {
__ACCUM_OP__
            return true;

        default:
            return false;
    }
}

""".replace('__TIMESTAMP__', stamp)\
   .replace('__NOPS__', '\n'.join(nops))\
   .replace('__TEXT__', '\n'.join(text))\
   .replace('__SYS_OP__', '\n'.join(sys_op))\
   .replace('__ELEM_OP__', '\n'.join(elem_op))\
   .replace('__REDUCE_OP__', '\n'.join(reduce_op))\
   .replace('__ACCUM_OP__', '\n'.join(accum_op))

def get_timestamp(f):
    st = os.stat(f)
    atime = st[stat.ST_ATIME] #access time
    mtime = st[stat.ST_MTIME] #modification time
    return (atime,mtime)

def set_timestamp(f,timestamp):
    os.utime(f,timestamp)

def main(script_dir):

    # Save the newest timestamp of this file and the definition file.
    # We will set this timest
    timestamp = get_timestamp(os.path.join(script_dir,'gen_opcodes.py'))
    t = get_timestamp(os.path.join(script_dir,'opcodes.json'))
    timestamp = t if t[1] > timestamp[1] else timestamp

    # Read the opcode definitions from opcodes.json.
    opcodes = json.loads(open(os.path.join(script_dir,'opcodes.json')).read())

    # Write the header file
    headerfile  = gen_headerfile(opcodes)
    cfile       = gen_cfile(opcodes)

    name = os.path.join(script_dir,'..','..','include','bh_opcode.h')
    h = open(name,"w")
    h.write(headerfile)
    h.close()
    set_timestamp(name, timestamp)

    # Write the c file
    name = os.path.join(script_dir,'..','bh_opcode.cpp')
    h = open(name,"w")
    h.write(cfile)
    h.close()
    set_timestamp(name, timestamp)

if __name__ == "__main__":
    try:
        script_dir = os.path.abspath(os.path.dirname(__file__))
    except NameError:
        print "The build script cannot run interactively."
        sys.exit(-1)
    main(script_dir)
