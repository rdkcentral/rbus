#
#  If not stated otherwise in this file or this component's Licenses.txt file
#  the following copyright and licenses apply:
#
#  Copyright 2016 RDK Management
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#  

import xml.etree.ElementTree as ET
import sys

header_rbus_c = """/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 
 * code generated with: python """ + sys.argv[0] + """ """ + sys.argv[1] + """ """ + sys.argv[2] + """
 */

#include <rbus.h>
#include <rbus_context_helpers.h>
#include <rtMemory.h>
#include <rtLog.h>
#include <stdlib.h>
#include <string.h>

rbusHandle_t gBusHandle = NULL;
char const gComponentID[] = \""""  + sys.argv[2] + """\";

rbusError_t registerGeneratedDataElements(rbusHandle_t handle);

rbusError_t Sample_RegisterRow(char const* tableName, uint32_t instNum, char const* alias, void* context)
{
    rbusError_t rc;
    rc = rbusTable_registerRow(gBusHandle, tableName, instNum, alias);
    if(rc == RBUS_ERROR_SUCCESS)
        SetRowContext(tableName, instNum, alias, context);    
    return rc;    
}

rbusError_t Sample_UnregisterRow(char const* tableName, uint32_t instNum)
{
    rbusError_t rc;
    char rowName[RBUS_MAX_NAME_LENGTH];
    snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d", tableName, instNum);
    rc = rbusTable_unregisterRow(gBusHandle, rowName);
    if(rc == RBUS_ERROR_SUCCESS)
        RemoveRowContextByName(rowName);    
    return rc;    
}
"""  

header_rbus_h = """/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 
 * code generated with: python """ + sys.argv[0] + """ """ + sys.argv[1] + """ """ + sys.argv[2] + """
 */

#include <rbus.h>

"""  

header_app_c = """/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 
 * code generated with: python """ + sys.argv[0] + """ """ + sys.argv[1] + """ """ + sys.argv[2] + """
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include "%s_rbus.h"

int shutdown = 0;

void sigIntHandler(int sig)
{
    shutdown = 1;
    signal(SIGINT, SIG_DFL);
}

int main()
{
    rbusError_t err = RBUS_ERROR_SUCCESS;
    %s
    if(err != RBUS_ERROR_SUCCESS)
        return err;
    signal(SIGINT, sigIntHandler);
    while(!shutdown)
        usleep(100000);
    %s
    return 0;
}
"""  

ParamBool=0
ParamInt=1
ParamUlong=2
ParamString=3

ObjectPlain=0
ObjectStaticTable=1
ObjectDynamicTable=2,
ObjectWritableTable=3,
ObjectDynWritableTable=4

func_GetEntryCount=0
func_GetEntryStatus=1
func_GetEntry=2
func_AddEntry=3
func_DelEntry=4
func_IsUpdated=5
func_Synchronize=6
func_Lock=7
func_Unlock=8
func_CheckInstance=9
func_GetParamBoolValue=10
func_GetParamIntValue=11
func_GetParamUlongValue=12
func_GetParamStringValue=13
func_GetBulkParamValues=14
func_SetParamBoolValue=15
func_SetParamIntValue=16
func_SetParamUlongValue=17
func_SetParamStringValue=18
func_SetBulkParamValues=19
func_Validate=20
func_Commit=21
func_Rollback=22
func_Init=23
func_Async_Init=24
func_Unload=25
func_MemoryCheck=26
func_MemoryUsage=27
func_MemoryTable=28
func_IsObjSupported=29

def initObjectNames(objectMap):
  objectMap["object"]=0
  objectMap["staticTable"]=1 
  objectMap["dynamicTable"]=2 
  objectMap["writableTable"]=3 
  objectMap["dynWritableTable"]=4 

def initFuncNames(funcMap):
  funcMap["func_GetEntryCount"]=0
  funcMap["func_GetEntryStatus"]=1
  funcMap["func_GetEntry"]=2
  funcMap["func_AddEntry"]=3
  funcMap["func_DelEntry"]=4
  funcMap["func_IsUpdated"]=5
  funcMap["func_Synchronize"]=6
  funcMap["func_Lock"]=7
  funcMap["func_Unlock"]=8
  funcMap["func_CheckInstance"]=9
  funcMap["func_GetParamBoolValue"]=10
  funcMap["func_GetParamIntValue"]=11
  funcMap["func_GetParamUlongValue"]=12
  funcMap["func_GetParamStringValue"]=13
  funcMap["func_GetBulkParamValues"]=14
  funcMap["func_SetParamBoolValue"]=15
  funcMap["func_SetParamIntValue"]=16
  funcMap["func_SetParamUlongValue"]=17
  funcMap["func_SetParamStringValue"]=18
  funcMap["func_SetBulkParamValues"]=19
  funcMap["func_Validate"]=20
  funcMap["func_Commit"]=21
  funcMap["func_Rollback"]=22
  funcMap["func_Init"]=23
  funcMap["func_Async_Init"]=24
  funcMap["func_Unload"]=25
  funcMap["func_MemoryCheck"]=26
  funcMap["func_MemoryUsage"]=27
  funcMap["func_MemoryTable"]=28
  funcMap["func_IsObjSupported"]=29

class DataElem:
  def __init__(self, name, type, fnget, fnset, fnadd, fndel, fnsub, fnmeth):
    self.name=name
    self.type=type
    self.fnget=fnget
    self.fnset=fnset 
    self.fnadd=fnadd
    self.fndel=fndel
    self.fnsub=fnsub
    self.fnmeth=fnmeth

class Param:
  def __init__(self, pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify):
    self.pname=pname
    self.ptype=ptype
    self.pstrtype=pstrtype
    self.psyntax1=psyntax1
    self.psyntax2=psyntax2
    self.pwritable=pwritable
    self.pnotify=pnotify
    self.added=False

#printElement is only for debugging that we know how to write code that walks the tree 
def printElement(elem,depth):
  offset=""
  for i in range(depth):
    offset = offset + "  "
  if(len(elem)==0):
    if elem.text:
      print(offset+"<"+elem.tag+">"+elem.text+"</"+elem.tag+">")
    else:
      print(offset+"<"+elem.tag+">"+"</"+elem.tag+">")
  else:
    print(offset+"<"+elem.tag+">")
    for subelem in elem:
      printElement(subelem,depth+1)
    print(offset+"</"+elem.tag+">")

def createDataElements(path, getters, setters, fnget, fnset):
    global dataElems
    for getter in getters:
      for setter in setters:
        if setter.pwritable and getter.pname == setter.pname:
          dataElems.append(DataElem(path+"."+getter.pname, "RBUS_ELEMENT_TYPE_PROPERTY", fnget+"_rbus", fnset+"_rbus",  "NULL", "NULL", "NULL", "NULL"))
          getter.added = setter.added = True
      if not getter.added:
          dataElems.append(DataElem(path+"."+getter.pname, "RBUS_ELEMENT_TYPE_PROPERTY", fnget+"_rbus", "NULL",         "NULL", "NULL", "NULL", "NULL"))
    for setter in setters:
      if setter.pwritable and not setter.added:
          dataElems.append(DataElem(path+"."+getter.pname, "RBUS_ELEMENT_TYPE_PROPERTY", "NULL",        fnset+"_rbus",  "NULL", "NULL", "NULL", "NULL"))

def writeTable(elem,path):
  global funcMap
  name=""
  objectType=0
  maxInstances=False
  addName=""
  delName=""
  isupName=""
  syncName=""
  for subelem in elem:
    if subelem.tag=="name":
      name=subelem.text
    elif subelem.tag=="objectType":
      objectType = objectMap[subelem.text]
    elif subelem.tag=="maxInstance":
      maxInstances=subelem.text
    elif subelem.tag=="functions":
      for funelem in subelem:
        if funelem.tag=="func_AddEntry":
          addName = funelem.text
        elif funelem.tag=="func_DelEntry":
          delName = funelem.text
        elif funelem.tag=="func_IsUpdated":
          isupName = funelem.text
        elif funelem.tag=="func_Synchronize":
          syncName = funelem.text
  if(objectType == 0):
    return path, True, False
  isDynamic = False
  isWritable = True
  if objectType == 2 or objectType == 4:
    isDynamic = True
  if objectType == 1 or objectType == 2:
    isWritable = False
  # isWritable means consumers can add/remove rows
  # isDynamic means the provider will resync the rows when any query comes in
  # PandM has several dynWritableTable, which implies a consumer can add/remove rows but also the provider can resync all the rows
  #     How does pandm keep track of consumer added/removed rows when a resync occurs ? 
  if isWritable:
    if addName:
      fout_rbus_c.write("\nvoid* " + addName + "(void* ctx, uint32_t* instNum)\n{\n    return NULL;\n}\n")
      fout_rbus_c.write(
"""
static rbusError_t %s_rbus(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    HandlerContext context = GetTableContext(tableName);
    void* rowContext = %s(context.userData, instNum);
    if(!rowContext)
    {
        rtLog_Error("%s returned null row context");
        return RBUS_ERROR_BUS_ERROR;
    }
    SetRowContext(context.fullName, *instNum, aliasName, rowContext);
    return RBUS_ERROR_SUCCESS;
}
""" %(addName,addName,addName))
    if delName:
      fout_rbus_c.write("\nrbusError_t " + delName + "(void* ctx, void* inst)\n{\n    return RBUS_ERROR_SUCCESS;\n}\n")
      fout_rbus_c.write(
"""
static rbusError_t %s_rbus(rbusHandle_t handle, char const* rowName)
{
    HandlerContext context = GetHandlerContext(rowName);  
    void* rowContext = GetRowContext(context.fullName);
    int rc = %s(context.userData, rowContext);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rtLog_Error("%s failed");
        return RBUS_ERROR_BUS_ERROR;
    }
    RemoveRowContextByName(context.fullName);
    return RBUS_ERROR_SUCCESS;
}
""" % (delName,delName,delName)) 
  if isDynamic:
    if isupName:
      fout_rbus_c.write("\nbool " + isupName + "(void* ctx)\n{\n    return true;\n}\n")
    if syncName:
      fout_rbus_c.write("\nrbusError_t " + syncName + "(void* ctx)\n{\n    return RBUS_ERROR_SUCCESS;\n}\n")
  if addName:
    addName = addName+"_rbus"
  else:
    addName = "NULL"
  if delName:
    delName = delName+"_rbus"
  else:
    delName = "NULL"
  path=path+".{i}"
  dataElems.append(DataElem(path+".", "RBUS_ELEMENT_TYPE_TABLE", "NULL", "NULL", addName, delName, "NULL", "NULL"))
  return path, isWritable, isDynamic

def writeComment(func,params):
  fout_rbus_c.write("\n/*\n")
  fout_rbus_c.write(" * %s\n" % (func))
  for param in params:
    fout_rbus_c.write(" *    %s\n" % (param.pname))
  fout_rbus_c.write(" */")

def writeSimpleContextFuncDecl(func):
  fout_rbus_c.write("\nrbusError_t %s(void* ctx)\n{\n    return RBUS_ERROR_SUCCESS;\n}\n" % (func))

def writeGetter(func,params,stype1,stype2,sparams,isDynamic,isupFunc,syncFunc):
  #writeComment(func,params)

  fout_rbus_c.write(
"""
static rbusError_t %s_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);""" % (func))

  if isDynamic and len(isupFunc)>0 and len(syncFunc)>0:
    fout_rbus_c.write(
"""    
    rbusError_t ret;""")
  fout_rbus_c.write("\n\n");

  if isDynamic and len(isupFunc)>0 and len(syncFunc)>0:
    fout_rbus_c.write(
"""    if((ret = do_%s_%s(context)) != RBUS_ERROR_SUCCESS)
        return ret;

""" % (isupFunc,syncFunc))

  first = True;
  for param in params:
    fout_rbus_c.write(
"""    %sif(strcmp(context.name, "%s") == 0)
    {
        //rbusProperty_Set%s(property, YOUR_VALUE);
    }
""" % ("" if first else "else ", param.pname,stype2))
    first = False

  if not first:
    fout_rbus_c.write(
"""    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
""")

  fout_rbus_c.write(
"""
    return RBUS_ERROR_SUCCESS;
}
""")

def writeSetter(func,params,stype1,stype2,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc):
  hasWritableParam=False
  for param in params:
    if param.pwritable:
      hasWritableParam=True
  if not hasWritableParam:
    return
  #writeComment(func,params)
  type = stype1.split()[0];
  fout_rbus_c.write(
"""
rbusError_t %s_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);""" % (func))

  if isDynamic and len(isupFunc)>0 and len(syncFunc)>0:
    fout_rbus_c.write(
"""    
    rbusError_t ret;""")
  fout_rbus_c.write("\n\n");

  if isDynamic and len(isupFunc)>0 and len(syncFunc)>0:
    fout_rbus_c.write(
"""
    if((ret = do_%s_%s(context)) != RBUS_ERROR_SUCCESS)
        return ret;
""" % (isupFunc,syncFunc))

  first = True;
  for param in params:
    if param.pwritable:
      fout_rbus_c.write(
"""    %sif(strcmp(context.name, "%s") == 0)
    {
""" % ("" if first else "else ", param.pname))

      fout_rbus_c.write(
"""        %s%s%s val;
""" % ("const " if type == "char" else "", type, "*" if type == "char" else ""))

      fout_rbus_c.write(
"""
        rbusValueError_t verr = rbusProperty_Get%sEx(property, &val%s);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
""" % (stype2, (", NULL" if type == "char" else "")))
      first = False

  if not first:
    fout_rbus_c.write(
"""    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
""")

  if(len(validateFunc)>0 and len(commitFunc)>0 and len(rollbackFunc)>0):
    fout_rbus_c.write(
"""    
    if(opts->commit)
    {
      return do_%s_%s_%s(context.userData);
    }

""" % (validateFunc,commitFunc,rollbackFunc))
  fout_rbus_c.write(
"""    return RBUS_ERROR_SUCCESS;
}
""")

def writeDynamicTableSync(isupFunc, syncFunc):
  fout_rbus_c.write(
"""
int do_%s_%s(HandlerContext context)
{
    if(IsTimeToSyncDynamicTable(context.name))
    {
        if(%s(context.userData))
        {
            return %s(context.userData);
        }
    }
    return 0;
}
""" % (isupFunc, syncFunc, isupFunc, syncFunc))

def writeValidateRollbackCommitHandlers(validateFunc, commitFunc, rollbackFunc):
  writeSimpleContextFuncDecl(validateFunc)
  writeSimpleContextFuncDecl(commitFunc)
  writeSimpleContextFuncDecl(rollbackFunc)
  fout_rbus_c.write(
"""
rbusError_t do_%s_%s_%s(void* context)
{
    if(%s(context) == 0)
    {
        if(%s(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(%s(context) == 0)
            return RBUS_ERROR_INVALID_INPUT;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}
""" % (validateFunc, commitFunc, rollbackFunc, validateFunc, commitFunc, rollbackFunc))

def writeSetterPrefix(func,params):
  #writeComment(func,params)
  fout_rbus_c.write("rbusError_t %s_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)\n{\n" % (func))

def writeBoolGetter(func, params, isDynamic,isupFunc,syncFunc):
  writeGetter(func, params, "bool value", "Boolean", "&value", isDynamic, isupFunc,syncFunc)

def writeIntGetter(func, params, isDynamic,isupFunc,syncFunc):
  writeGetter(func, params, "int32_t value", "Int32", "&value", isDynamic, isupFunc,syncFunc)

def writeUlongGetter(func, params, isDynamic,isupFunc,syncFunc):
  writeGetter(func, params, "uint32_t value", "UInt32", "&value", isDynamic, isupFunc,syncFunc)

def writeStringGetter(func, params, isDynamic,isupFunc,syncFunc):
  writeGetter(func, params, "char value[256];\n    uint32_t ulen = 256", "String", "value, &ulen", isDynamic, isupFunc, syncFunc)

def writeBoolSetter(func, params,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc):
  writeSetter(func, params,"bool value", "Boolean",validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc);

def writeIntSetter(func, params,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc):
  writeSetter(func, params, "int32_t value", "Int32",validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)

def writeUlongSetter(func, params,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc):
  writeSetter(func, params, "uint32_t value", "UInt32",validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)

def writeStringSetter(func, params,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc):
  writeSetter(func, params, "char value[256]", "String",validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)

#Possible FIXME
#from the dm pack there was this issue which i'm not solving in this script:
#I saw this weird case happen 2 times in TR181-USGv2.XML
#one param was prefixed CodeBigFirst but its other sibling was prefixed CodeBig_First*
#In another object, all the sibling params were were GreTunnelIf but 1 oddball was GreTunnelIF (note the cap I)
#I don't know if this was put in the xml intentionally or this is human error
#to be safe I will preserve what is there by switching to the new unexpected prefix
def writeParams(elem,path,isWritable,isDynamic):
  getBoolFunc=""
  getIntFunc=""
  getUlongFunc=""
  getStrFunc=""
  setBoolFunc=""
  setIntFunc=""
  setUlongFunc=""
  setStrFunc=""
  validateFunc=""
  commitFunc=""
  rollbackFunc=""
  isupFunc=""
  syncFunc=""
  getBoolParams=[]
  getIntParams=[]
  getUlongParams=[]
  getStrParams=[]
  setBoolParams=[]
  setIntParams=[]
  setUlongParams=[]
  setStrParams=[]
  validateParams=[]
  commitParams=[]
  rollbackParams=[]
  for subelem in elem:
    #get function names
    if subelem.tag=="functions":
      for funelem in subelem:
        if funelem.tag=="func_GetParamBoolValue":
          getBoolFunc = funelem.text
        elif funelem.tag=="func_GetParamIntValue":
          getIntFunc = funelem.text
        elif funelem.tag=="func_GetParamUlongValue":
          getUlongFunc = funelem.text
        elif funelem.tag=="func_GetParamStringValue":
          getStrFunc = funelem.text
        elif funelem.tag=="func_SetParamBoolValue":
          setBoolFunc = funelem.text
        elif funelem.tag=="func_SetParamIntValue":
          setIntFunc = funelem.text
        elif funelem.tag=="func_SetParamUlongValue":
          setUlongFunc = funelem.text
        elif funelem.tag=="func_SetParamStringValue":
          setStrFunc = funelem.text
        elif funelem.tag=="func_Validate":
          validateFunc = funelem.text
        elif funelem.tag=="func_Commit":
          commitFunc = funelem.text
        elif funelem.tag=="func_Rollback":
          rollbackFunc = funelem.text
        elif funelem.tag=="func_IsUpdated":
          isupFunc = funelem.text
        elif funelem.tag=="func_Synchronize":
          syncFunc = funelem.text
    #read parameter data
    elif subelem.tag=="parameters":
      for parelem in subelem:
        pname=""
        ptype=-1
        pstrtype=""
        psyntax1=""
        psyntax2=""
        pwritable=False
        pnotify=-1
        for subelem2 in parelem:
          if subelem2.tag=="name":
            pname=subelem2.text
          elif subelem2.tag=="type":
            pstrtype=subelem2.text
            if pstrtype=="boolean":
              ptype=ParamBool
              psyntax1="bool"
            elif pstrtype=="int":
              ptype=ParamInt
              psyntax1="int"
            elif pstrtype=="unsignedInt":
              ptype=ParamUlong
              psyntax1="uint32"
            elif subelem2.text[:6]=="string":
              ptype=ParamString
              psyntax1="string"
          elif subelem2.tag=="syntax":
            psyntax2=subelem2.text
          elif subelem2.tag=="writable":
            if subelem2.text == "true":
              pwritable=True
          elif subelem2.tag=="notify":
            if subelem2.text == "off":
              pnotify=0
            elif subelem2.text == "on":
              pnotify=1
        #record params
        if(ptype==ParamBool):
          getBoolParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
          setBoolParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
        if(ptype==ParamInt):
          getIntParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
          setIntParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
        if(ptype==ParamUlong):
          getUlongParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
          setUlongParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
        if(ptype==ParamString):
          getStrParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
          setStrParams.append(Param(pname, ptype, pstrtype, psyntax1, psyntax2, pwritable, pnotify))
  #dynamics
  if(isDynamic and len(isupFunc)>0 and len(syncFunc)>0):
    writeDynamicTableSync(isupFunc, syncFunc)
  #write getters
  if(len(getBoolFunc)>0 and len(getBoolParams)>0):
    writeBoolGetter(getBoolFunc,getBoolParams,isDynamic,isupFunc,syncFunc)
  if(len(getIntFunc)>0 and len(getIntParams)>0):
    writeIntGetter(getIntFunc, getIntParams, isDynamic,isupFunc,syncFunc)
  if(len(getUlongFunc)>0 and len(getUlongParams)>0):
    writeUlongGetter(getUlongFunc, getUlongParams, isDynamic,isupFunc,syncFunc)
  if(len(getStrFunc)>0 and len(getStrParams)>0):
    writeStringGetter(getStrFunc, getStrParams, isDynamic,isupFunc,syncFunc)
  #committers
  if(len(validateFunc)>0 and len(commitFunc)>0 and len(rollbackFunc)>0):
    writeValidateRollbackCommitHandlers(validateFunc, commitFunc, rollbackFunc)
  #write setters
  if(len(setBoolFunc)>0 and len(setBoolParams)>0):
    writeBoolSetter(setBoolFunc, setBoolParams,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)
  if(len(setIntFunc)>0 and len(setIntParams)>0):
    writeIntSetter(setIntFunc, setIntParams,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)
  if(len(setUlongFunc)>0 and len(setUlongParams)>0):
    writeUlongSetter(setUlongFunc, setUlongParams,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)
  if(len(setStrFunc)>0 and len(setStrParams)>0):
    writeStringSetter(setStrFunc, setStrParams,validateFunc,commitFunc,rollbackFunc,isDynamic,isupFunc,syncFunc)

  createDataElements(path, getBoolParams, setBoolParams, getBoolFunc, setBoolFunc)
  createDataElements(path, getIntParams, setIntParams, getIntFunc, setIntFunc)
  createDataElements(path, getUlongParams, setUlongParams, getUlongFunc, setUlongFunc)
  createDataElements(path, getStrParams, setStrParams, getStrFunc, setStrFunc)

def writeRegDataElements():
  global dataElems
  if len(dataElems) == 0:
    return
  fout_rbus_c.write(
"""
rbusError_t registerGeneratedDataElements(rbusHandle_t handle)
{
    rbusError_t rc;
    static rbusDataElement_t dataElements["""+str(len(dataElems))+"""] = {
""")
  first = True
  for el in dataElems:
    if not first:
      fout_rbus_c.write(",\n")
    first = False
    fout_rbus_c.write("        {\"%s\", %s, {%s, %s, %s, %s, %s, %s}}" % (el.name, el.type, el.fnget, el.fnset, el.fnadd, el.fndel, el.fnsub, el.fnmeth));
  fout_rbus_c.write("""
    };
    rc = rbus_regDataElements(handle, """+str(len(dataElems))+""", dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rtLog_Error("rbus_regDataElements failed");
    }
    return rc;
}
""")

def codeGenObject(elem,path):
  for subelem in elem:
    if subelem.tag=="name":
      path = path + "." + subelem.text
  path, isWritable, isDynamic = writeTable(elem,path)
  writeParams(elem,path,isWritable,isDynamic)
  for subelem in elem:
    if subelem.tag=="objects":
      codeGenElement(subelem,path)
   
def codeGenElement(elem,path):
  for subelem in elem:
    if subelem.tag=="object":
      codeGenObject(subelem,path)
    else:
      codeGenElement(subelem,path)

def getLibraryFuncs(elem):
  initFunc=""
  unloadFunc=""
  for subelem in elem:
    if subelem.tag=="library":
      for func in subelem:
        if func.tag=="func_Init":
          initFunc = func.text
        elif func.tag=="func_Unload":
          unloadFunc = func.text
      return initFunc,unloadFunc
    else:
      initFunc,unloadFunc=getLibraryFuncs(subelem)
  return initFunc,unloadFunc

def writeInitAndUnload(initFunc,unloadFunc):
  if len(initFunc):
    fout_rbus_h.write("rbusError_t %s();\n" % (initFunc))
    fout_rbus_c.write(
"""
rbusError_t %s()
{
    rbusError_t rc;
    rbusHandle_t rbusHandle = NULL;
    
    rc = rbus_open(&rbusHandle, gComponentID);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        return rc;
    }
    rc = registerGeneratedDataElements(rbusHandle);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rbus_close(rbusHandle);
        return rc;
    }
    Context_Init();
    gBusHandle = rbusHandle;
    return RBUS_ERROR_SUCCESS;
}
""" % (initFunc))
  if len(unloadFunc):
    fout_rbus_h.write("rbusError_t %s();\n" % (unloadFunc))
    fout_rbus_c.write(
"""
rbusError_t %s()
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    if(gBusHandle)
    {
        rc = rbus_close(gBusHandle);
        gBusHandle = NULL;
    }
    Context_Release();
    return rc;    
}
""" % (unloadFunc));

def writeTestApp(initFunc,unloadFunc):
  if initFunc:
    initFunc = "err = %s();" % (initFunc);
  else:
    initFunc="/* init component here */";
  if unloadFunc:
    unloadFunc = "err = %s();" % (unloadFunc);
  else:
    unloadFunc="/* unload component here */";
  fout_app_c.write(header_app_c % (sys.argv[2], initFunc, unloadFunc))

def codeGen(xmlFile):
  tree = ET.parse(xmlFile)
  root = tree.getroot()
  fout_rbus_c.write(header_rbus_c)
  fout_rbus_h.write(header_rbus_h)
  initFunc,unloadFunc=getLibraryFuncs(root)
  if len(initFunc) == 0:
    initFunc=sys.argv[2]+"_Init"
  if len(unloadFunc) == 0:
    unloadFunc=sys.argv[2]+"_Unload"
  writeInitAndUnload(initFunc,unloadFunc)
  codeGenElement(root,"Device")
  writeRegDataElements()
  writeTestApp(initFunc,unloadFunc)

if len(sys.argv) != 3:
  print("usage: python ccsp_xml_code_gen.py in_xml_file out_c_file")
  quit();

funcMap={}
objectMap={}
dataElems=[]
fout_rbus_c = open(sys.argv[2]+"_rbus.c", "w")
fout_rbus_h = open(sys.argv[2]+"_rbus.h", "w")
fout_app_c = open(sys.argv[2]+"_app.c", "w")

initFuncNames(funcMap)
initObjectNames(objectMap)

codeGen(sys.argv[1])

fout_rbus_c.close();  
fout_rbus_h.close();  
fout_app_c.close();


