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
#include "rbus_element.h"
#include "../common/test_macros.h"

#define MD5_LEN 32

int getDurationElementTree()
{
    return 1;
}

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
    int pass = !strcmp(md5, correctmd5 );
    printf("%s: %s path=%s md5=%s\n", pass ? "PASS" : "FAIL", name, path, md5);
    TALLY(pass);
}

void insertElem(elementNode* root, char* path, rbusElementType_t type)
{
    rbusDataElement_t elem = {path, type, {NULL}};
    insertElement(root, &elem);
}

void insertElemWithGetter(elementNode* root, char* path, rbusElementType_t type)
{
    rbusDataElement_t elem = {path, type, {(rbusGetHandler_t)!NULL, NULL, NULL, NULL, NULL, NULL}};
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

void testRetrieveElement(elementNode* root, const char* query, const char* expectedFullName)
{
    elementNode* el = retrieveElement(root, query);
    int pass;

    if(el)
    {
        if(expectedFullName)
        {
            pass = !strcmp(el->fullName, expectedFullName);
            printf("%s: retrieveElement=%s expected=%s actual=%s\n", pass ? "PASS" : "FAIL", query, expectedFullName, el->fullName);
        }
        else
        {
            printf("FAIL: retrieveElement=%s expected=NULL actual=%s\n", query, el->fullName);
            pass = 0;
        }
    }
    else
    {
        if(expectedFullName)
        {
            printf("FAIL: retrieveElement=%s expected=%s actual=NULL\n", query, expectedFullName);
            pass = 0;
        }
        else
        {
            printf("PASS: retrieveElement=%s expected=NULL actual=NULL\n", query);
            pass = 1;
        }
    }
    TALLY(pass);
}

void testRetrieveInstanceElement(elementNode* root, const char* query, const char* expectedFullName)
{
    elementNode* el = retrieveInstanceElement(root, query);
    int pass;

    if(el)
    {
        if(expectedFullName)
        {
            pass = !strcmp(el->fullName, expectedFullName);
            printf("%s: retrieveInstanceElement=%s expected=%s actual=%s\n", pass ? "PASS" : "FAIL", query, expectedFullName, el->fullName);
        }
        else
        {
            printf("FAIL: retrieveInstanceElement=%s expected=NULL actual=%s\n", query, el->fullName);
            pass = 0;
        }
    }
    else
    {
        if(expectedFullName)
        {
            printf("FAIL: retrieveInstanceElement=%s expected=%s actual=NULL\n", query, expectedFullName);
            pass = 0;
        }
        else
        {
            printf("PASS: retrieveInstanceElement=%s expected=NULL actual=NULL\n", query);
            pass = 1;
        }
    }
    TALLY(pass);
}

void testElementTree(rbusHandle_t handle, int* countPass, int* countFail)
{
    elementNode* root;
    (void)handle;

    /*create and fill tree*/
    root = getEmptyElementNode();
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

    /*add a table with getter (used by MtaAgent/TR104)*/
    insertElemWithGetter(root, "Device.Foo.TableWithGet.{i}", RBUS_ELEMENT_TYPE_TABLE);
    insertElem(root, "Device.Foo.TableWithGet.{i}.Prop1", RBUS_ELEMENT_TYPE_PROPERTY);
    checkmd5("rbus_test_elem_tree_4", root, "8805db4ea8566ec596cec25beed5c478");

    /*test queries*/

    /*test positive retrieveElement*/
    testRetrieveElement(root, "Device", "Device");
    testRetrieveElement(root, "Device.", "Device");
    testRetrieveElement(root, "Device.Prop2", "Device.Prop2");
    testRetrieveElement(root, "Device.Foo", "Device.Foo");
    testRetrieveElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1");
    testRetrieveElement(root, "Device.Foo.Prop2", "Device.Foo.Prop2");
    testRetrieveElement(root, "Device.Foo.Table1", "Device.Foo.Table1");
    testRetrieveElement(root, "Device.Foo.Table1.", "Device.Foo.Table1");
    testRetrieveElement(root, "Device.Foo.Table1.{i}", "Device.Foo.Table1.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.", "Device.Foo.Table1.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop2", "Device.Foo.Table1.{i}.Prop2");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Obj.Prop1", "Device.Foo.Table1.{i}.Obj.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Obj.Table2", "Device.Foo.Table1.{i}.Obj.Table2");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Obj.Table2.", "Device.Foo.Table1.{i}.Obj.Table2");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.3.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.4.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.1.Obj", "Device.Foo.Table1.{i}.Obj");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj", "Device.Foo.Table1.{i}.Obj");
    testRetrieveElement(root, "Device.Foo.Table1.3.Obj", "Device.Foo.Table1.{i}.Obj");
    testRetrieveElement(root, "Device.Foo.Table1.1.Obj.Table2.{i}", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.{i}", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.3.Obj.Table2.{i}", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.1.Obj.Table2.1", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.1.Obj.Table2.2", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.1", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.2", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.[ten]", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.[eleven]", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.TableWithGet", "Device.Foo.TableWithGet");
    testRetrieveElement(root, "Device.Foo.TableWithGet.", "Device.Foo.TableWithGet");
    testRetrieveElement(root, "Device.Foo.TableWithGet.{i}", "Device.Foo.TableWithGet.{i}");
    testRetrieveElement(root, "Device.Foo.TableWithGet.{i}.", "Device.Foo.TableWithGet.{i}");
    testRetrieveElement(root, "Device.Foo.TableWithGet.{i}.Prop1", "Device.Foo.TableWithGet.{i}.Prop1");

    /* test more positive cases where instances don't exist but the element should still get retrieved based on the pattern
     * ie. retrieveElement does not verify instance exist, it simply matches the pattern back to the registration element*/
    testRetrieveElement(root, "Device.Foo.Table1.4.Prop1", "Device.Foo.Table1.{i}.Prop1");
    testRetrieveElement(root, "Device.Foo.Table1.1.Obj.Table2.3", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.3", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.[apple]", "Device.Foo.Table1.{i}.Obj.Table2.{i}");
    testRetrieveElement(root, "Device.Foo.Table1.2.Obj.Table2.[orange]", "Device.Foo.Table1.{i}.Obj.Table2.{i}");

    /*test negative retrieveElement*/
    testRetrieveElement(root, "{i}", NULL);
    testRetrieveElement(root, "Device2", NULL);
    testRetrieveElement(root, "Device.{i}", NULL);
    testRetrieveElement(root, "Device.Bar", NULL);
    testRetrieveElement(root, "Device.Foo.Table1.{i}.Prop3", NULL);
    testRetrieveElement(root, "Device.Foo.Table1.{i}.{i}", NULL);

    /*test positive retrieveInstanceElement*/
    testRetrieveInstanceElement(root, "Device", "Device");
    testRetrieveInstanceElement(root, "Device.", "Device");
    testRetrieveInstanceElement(root, "Device.Prop2", "Device.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo", "Device.Foo");
    testRetrieveInstanceElement(root, "Device.Foo.Prop1", "Device.Foo.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.Prop2", "Device.Foo.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1", "Device.Foo.Table1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.", "Device.Foo.Table1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop1", "Device.Foo.Table1.1.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop1", "Device.Foo.Table1.2.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.3.Prop1", "Device.Foo.Table1.3.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.4.Prop1", "Device.Foo.Table1.4.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop2", "Device.Foo.Table1.1.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Prop2", "Device.Foo.Table1.2.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.3.Prop2", "Device.Foo.Table1.3.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.4.Prop2", "Device.Foo.Table1.4.Prop2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj", "Device.Foo.Table1.1.Obj");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj", "Device.Foo.Table1.2.Obj");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2", "Device.Foo.Table1.1.Obj.Table2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.", "Device.Foo.Table1.1.Obj.Table2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2", "Device.Foo.Table1.2.Obj.Table2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.", "Device.Foo.Table1.2.Obj.Table2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.3.Obj.Table2", "Device.Foo.Table1.3.Obj.Table2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.1", "Device.Foo.Table1.1.Obj.Table2.1");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.2", "Device.Foo.Table1.1.Obj.Table2.2");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.10", "Device.Foo.Table1.2.Obj.Table2.10");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.11", "Device.Foo.Table1.2.Obj.Table2.11");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[ten]", "Device.Foo.Table1.2.Obj.Table2.10");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[eleven]", "Device.Foo.Table1.2.Obj.Table2.11");

    /*test more positive cases where the registration element can be retrieved through the retrieveInstanceElement api*/
    testRetrieveInstanceElement(root, "Device.Foo.Table1.{i}", "Device.Foo.Table1.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}", "Device.Foo.Table1.{i}.Obj.Table2.{i}");

    /*test more positive cases where table with getter set.  It should always return the registration elements
      as a getter on the table implies no rows will be registered or we ignore them if they are */
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet", "Device.Foo.TableWithGet");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.", "Device.Foo.TableWithGet");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.{i}", "Device.Foo.TableWithGet.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.{i}.", "Device.Foo.TableWithGet.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.{i}.Prop1", "Device.Foo.TableWithGet.{i}.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.1", "Device.Foo.TableWithGet.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.1.", "Device.Foo.TableWithGet.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.1.Prop1", "Device.Foo.TableWithGet.{i}.Prop1");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.2", "Device.Foo.TableWithGet.{i}");
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.2.Prop1", "Device.Foo.TableWithGet.{i}.Prop1");


    /*test negative retrieveInstanceElement*/
    testRetrieveInstanceElement(root, "Device2", NULL);
    testRetrieveInstanceElement(root, "Device.Prop3", NULL);
    testRetrieveInstanceElement(root, "Device.Foo2", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.5", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.5.Prop1", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Prop4", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.3", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.12", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.[ten]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.1.Obj.Table2.[eleven]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[te]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[tens]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[eleve]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[elevens]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.Table1.2.Obj.Table2.[]", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.{i}.Prop2", NULL);
    testRetrieveInstanceElement(root, "Device.Foo.TableWithGet.1.Prop2", NULL);

    /*now remove things in order added and verify we go back to exact previous state (md5s match)*/
    removeElem(root, "Device.Foo.TableWithGet.{i}.Prop1");
    removeElem(root, "Device.Foo.TableWithGet.{i}");
    checkmd5("rbus_test_elem_tree_5", root, "e93d4d90842913807019b4f3986f63ee");

    delRow(root,"Device.Foo.Table1.2.Obj.Table2.11" );
    delRow(root,"Device.Foo.Table1.2.Obj.Table2.10" );
    delRow(root,"Device.Foo.Table1.4" );
    removeElem(root, "Device.Foo.Table1.{i}.Prop2");
    removeElem(root, "Device.Foo.Prop2");
    removeElem(root, "Device.Prop2");
    checkmd5("rbus_test_elem_tree_6", root, "71e78112cd774d456ecb6a8d6b91f5aa");

    delRow(root,"Device.Foo.Table1.1.Obj.Table2.2" );
    delRow(root,"Device.Foo.Table1.1.Obj.Table2.1" );
    delRow(root,"Device.Foo.Table1.3" );
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.Obj.Table2.{i}.");
    checkmd5("rbus_test_elem_tree_7", root, "731c36fb7fefca9ae78200eb2eecb117");

    removeElem(root, "Device.Foo.Table1.{i}.Prop1");
    removeElem(root, "Device.Foo.Table1.{i}.");
    removeElem(root, "Device.Foo.Prop1");
    checkmd5("rbus_test_elem_tree_8", root, "57455ea5720ef5cdcf8cd4ef898725a2");

    freeElementNode(root);

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ElementTree");
}
