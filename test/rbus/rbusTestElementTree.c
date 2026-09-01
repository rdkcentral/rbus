/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <rbus.h>
#include <rtLog.h>
#include "../src/rbus_element.h"

#define MD5_LEN 32

int md5sum(const char* file, char* md5)
{
    char cmd[256];
    snprintf(cmd, 255, "md5sum %s 2>/dev/null", file);

    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    int i, ch;
    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++)
    {
        *md5++ = ch;
    }
    *md5 = '\0';
    pclose(p);
    return i == MD5_LEN;
}

void checkmd5(const char* name, elementNode* root, const char* correctmd5)
{
    char path[256];
    snprintf(path, 255, "/tmp/%s.txt", name);
    FILE* f = fopen(path, "w");
    fprintRegisteredElements(f, root, 0);
    fclose(f);
    char md5[MD5_LEN+1];
    md5sum(path, md5);
    printf("md5 %s %s\n", path, md5);
    printf("%s %s\n", name, strcmp(md5, correctmd5 ) == 0 ? "PASS" : "FAIL");
}

void insertElem(elementNode* root, char* path, rbusElementType_t type)
{
    rbusDataElement_t elem = {path, type, {NULL}};
    insertElement(root, &elem);
}

void removeElem(elementNode* root, const char* path)
{
    elementNode* node = retrieveElement(root, path);
    removeElement(node);
}

void addRow(elementNode* root, const char* path, uint32_t insNum, const char* alias)
{
    elementNode* node = retrieveInstanceElement(root, path);
    instantiateTableRow(node, insNum, alias);
}

void delRow(elementNode* root, const char* path)
{
    elementNode* node = retrieveInstanceElement(root, path);
    deleteTableRow(node);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    //rtLog_SetLevel(RT_LOG_DEBUG);

    elementNode* root = getEmptyElementNode();
    root->name = strdup("root");
    root->fullName = strdup("root");
    checkmd5("rbus_test_elem_tree_0", root, "57455ea5720ef5cdcf8cd4ef898725a2");

    insertElem(root, "Device.Foo.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 1, NULL);
    addRow(root, "Device.Foo.Table1.", 2, NULL);
    checkmd5("rbus_test_elem_tree_1", root, "731c36fb7fefca9ae78200eb2eecb117");

    insertElem(root, "Device.Foo.Table1.{i}.Obj.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.", RBUS_ELEMENT_TYPE_TABLE);
    addRow(root, "Device.Foo.Table1.", 3, NULL);
    addRow(root, "Device.Foo.Table1.1.Obj.Table2.", 1, NULL);
    addRow(root, "Device.Foo.Table1.1.Obj.Table2.", 2, NULL);
    checkmd5("rbus_test_elem_tree_2", root, "71e78112cd774d456ecb6a8d6b91f5aa");

    insertElem(root, "Device.Prop2", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Prop2", RBUS_ELEMENT_TYPE_PROPERTY);
    insertElem(root, "Device.Foo.Table1.{i}.Prop2", RBUS_ELEMENT_TYPE_PROPERTY);
    addRow(root, "Device.Foo.Table1.", 4, NULL);
    addRow(root, "Device.Foo.Table1.2.Obj.Table2.", 10, "ten");
    addRow(root, "Device.Foo.Table1.2.Obj.Table2.", 11, "eleven");
    checkmd5("rbus_test_elem_tree_3", root, "e93d4d90842913807019b4f3986f63ee");

    /*now remove things in order added and verify we go back to exact previous state (md5s match)*/
    delRow(root,"Device.Foo.Table1.2.Obj.Table2.11" );
    delRow(root,"Device.Foo.Table1.2.Obj.Table2.10" );
    delRow(root,"Device.Foo.Table1.4" );
    removeElem(root, "Device.Foo.Table1.{i}.Prop2");
    removeElem(root, "Device.Foo.Prop2");
    removeElem(root, "Device.Prop2");
    checkmd5("rbus_test_elem_tree_4", root, "71e78112cd774d456ecb6a8d6b91f5aa");

    delRow(root,"Device.Foo.Table1.1.Obj.Table2.2" );
    delRow(root,"Device.Foo.Table1.1.Obj.Table2.1" );
    delRow(root,"Device.Foo.Table1.3" );
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.");
    checkmd5("rbus_test_elem_tree_5", root, "731c36fb7fefca9ae78200eb2eecb117");

    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");
    checkmd5("rbus_test_elem_tree_6", root, "57455ea5720ef5cdcf8cd4ef898725a2");

    freeElementNode(root);
    return 0;
}
