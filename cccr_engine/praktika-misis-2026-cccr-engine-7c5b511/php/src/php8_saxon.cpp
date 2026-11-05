#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php8_saxon.h"

#ifdef COMPILE_DL_SAXON
extern "C" {
ZEND_GET_MODULE(saxon)
}
#endif

zend_object_handlers saxonProcessor_object_handlers;

zend_object_handlers xdmValue_object_handlers;
zend_object_handlers xdmItem_object_handlers;
zend_object_handlers xdmNode_object_handlers;
zend_object_handlers xdmAtomicValue_object_handlers;
zend_object_handlers xdmFunctionItem_object_handlers;
zend_object_handlers xdmMap_object_handlers;
zend_object_handlers xdmArray_object_handlers;
zend_object_handlers sequenceIterator_object_handlers;

zend_class_entry *saxonProcessor_ce;

zend_class_entry *xdmValue_ce;
zend_class_entry *xdmItem_ce;
zend_class_entry *xdmNode_ce;
zend_class_entry *xdmAtomicValue_ce;
zend_class_entry *xdmFunctionItem_ce;
zend_class_entry *xdmMap_ce;
zend_class_entry *xdmArray_ce;
zend_class_entry *sequenceIterator_ce;

zend_class_entry *xsltExecutable_ce;
zend_class_entry *xsdSchema_ce;

void _decrementRefCount(std::map<std::string, XdmValue *> parameters){
    for (std::map<std::string, XdmValue *>::iterator itr = parameters.begin();
         itr != parameters.end(); itr++) {
      XdmValue *value = itr->second;
      if (value != nullptr) {
        value->decrementRefCount();
        if (value->getRefCount() < 1) {
          delete value;
          value = nullptr;
        }
      }
    }
}

void _decrementRefCountForValue(XdmValue * value){
    if(value != nullptr) {
        value->decrementRefCount();
        if(value->getRefCount() < 1) {
            delete value;
            value = nullptr;
        }
    }
}

#include "php8_SchemaValidator.cpp"
#include "php8_DocumentBuilder.cpp"
#include "php8_XPathProcessor.cpp"
#include "php8_XQueryProcessor.cpp"
#include "php8_Xslt30Processor.cpp"
#include "php8_XsltExecutable.cpp"
#include "php8_XsdCompiler.cpp"
#include "php8_XsdSchema.cpp"

void SaxonProcessor_free_storage(zend_object *object) {

  // std::cerr<<"_free_Storage called php-saxonProc"<<std::endl;

  saxonProcessor_object *obj;

  obj = (saxonProcessor_object *)((char *)object -
                                  XtOffsetOf(saxonProcessor_object, std));

  SaxonProcessor *saxonProc = obj->saxonProcessor;
  if (saxonProc != nullptr) {
    // SaxonProcessor::release();
    delete saxonProc;
  }
  zend_object_std_dtor(object);
}

void SaxonProcessor_destroy_storage(zend_object *pobj) {

  // php_error(E_WARNING,"_destroy_Storage called php-saxonProc");
  //   saxonProcessor_object *obj;
  // zend_object* pobj = Z_OBJ_P(getThis());
  /*  saxonProcessor_object * obj = (saxonProcessor_object *)((char *)pobj -
     XtOffsetOf(saxonProcessor_object, std));

     SaxonProcessor * saxonProc= obj->saxonProcessor;
     if(saxonProc != nullptr) {
         SaxonProcessor::release();
         delete saxonProc;
     }*/

  zend_objects_destroy_object(pobj);
}

zend_object *saxonProcessor_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  saxonProcessor_object *obj = (saxonProcessor_object *)ecalloc(
      1, sizeof(saxonProcessor_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &saxonProcessor_object_handlers;

  return &obj->std;
}

PHP_METHOD(SaxonProcessor, __construct) {
  if (ZEND_NUM_ARGS() > 2) {
    WRONG_PARAM_COUNT;
  }

  char *cwdi = nullptr;
  bool license = false;
  size_t len1;
  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "b", &license) == FAILURE) {
    RETURN_NULL();
  }

  if (ZEND_NUM_ARGS() > 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "bs", &license, &cwdi, &len1) ==
          FAILURE) {
    RETURN_NULL();
  }

  zval *object = getThis();
  SaxonProcessor *saxonProc = nullptr;
  zend_object *zobj = Z_OBJ_P(object);

  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)zobj -
                                XtOffsetOf(saxonProcessor_object, std));

  saxonProc = new SaxonProcessor(license);

  obj->saxonProcessor = saxonProc;
  // std::cerr<<"SaxonProcessorConstructor cp1"<<std::endl;
  if (cwdi == nullptr) {
// std::cerr<<"SaxonProcessorConstructor cp1-1"<<std::endl;
#if !(defined(__linux__) || (defined(__APPLE__) && defined(__MACH__)))
    TCHAR s[256];

    // --
    DWORD a = GetCurrentDirectory(256, s);
    const size_t newsize = wcslen(s) * 2;
    char *cwd = new char[newsize];
    wcstombs_s(0, cwd, newsize, s, _TRUNCATE);
    // -- code above returns the apache installation directory as the CWD

    char *cwd2;

    // php_error(E_WARNING,cwd2);

    saxonProc->setcwd(cwd);

    delete[] cwd;
#else
    char cwd[256];

    VCWD_GETCWD(cwd, sizeof(cwd));
    saxonProc->setcwd(cwd);
#endif
    delete cwdi;

  } else {
    saxonProc->setcwd(cwdi);
  }
}

PHP_METHOD(SaxonProcessor, __destruct) {
  // php_error(E_WARNING,"__destruct called php-saxonProc");
  // std::cerr<<"__destruct call saxonProc"<<std::endl;
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  SaxonProcessor *saxonProc = obj->saxonProcessor;
  if (saxonProc != nullptr) {
    // SaxonProcessor::release();
    // delete saxonProc;
  }
}

PHP_METHOD(SaxonProcessor, release) {
  // php_error(E_WARNING,"__destruct called php-saxonProc");
  //   std::cerr<<"__destruct call saxonProc"<<std::endl;
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  SaxonProcessor *saxonProc = obj->saxonProcessor;
  if (saxonProc != nullptr) {
    SaxonProcessor::release();
  }
}

PHP_METHOD(SaxonProcessor, createXdmMapValue) {
  SaxonProcessor *proc = nullptr;
  HashTable *arr_hash;

  zval *items_zval;
  zval *val;
  long num_key;
  XdmValue **arguments = nullptr;
  XdmAtomicValue **c_keys = nullptr;
  int argument_length = 0;
  zend_string *key;

  if (ZEND_NUM_ARGS() != 3) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &items_zval) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  SaxonProcessor *saxonProc = obj->saxonProcessor;
  arr_hash = Z_ARRVAL_P(items_zval);
  argument_length = zend_hash_num_elements(arr_hash);

  arguments = new XdmValue *[argument_length];
  c_keys = new XdmAtomicValue *[argument_length];

  if (saxonProc != nullptr) {
    ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {

      if (key != NULL) {
        const char *c_key = ZSTR_VAL(key);
        c_keys[num_key] = saxonProc->makeStringValue(c_key);

      } else {
        c_keys[num_key] = saxonProc->makeIntegerValue(num_key);
      }

      if (Z_TYPE_P(val) != IS_NULL) {
        const char *objName = ZSTR_VAL(Z_OBJCE_P(val)->name);

        if (strcmp(objName, "Saxon\\XdmNode") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmNode_object *nobj =
              (xdmNode_object *)((char *)ooth -
                                 XtOffsetOf(xdmNode_object, std));
          if (nobj != nullptr) {
            XdmValue *value = (XdmValue *)nobj->xdmNode;
            arguments[num_key] = value;
          }
        } else if (strcmp(objName, "Saxon\\XdmAtomicValue") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmAtomicValue_object *nobj =
              (xdmAtomicValue_object *)((char *)ooth -
                                        XtOffsetOf(xdmAtomicValue_object, std));
          if (nobj != nullptr) {
            XdmValue *value = (XdmValue *)nobj->xdmAtomicValue;
            arguments[num_key] = value;
          }
        } else if (strcmp(objName, "Saxon\\XdmValue") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmValue_object *nobj =
              (xdmValue_object *)((char *)ooth -
                                  XtOffsetOf(xdmValue_object, std));
          if (nobj != nullptr) {
            XdmValue *value = nobj->xdmValue;
            arguments[num_key] = value;
          }
        } else { // TODO error warning}
        }
      } else {
        // TODO error warning
      }
    }
    ZEND_HASH_FOREACH_END();

    XdmMap *result = saxonProc->makeMap3(c_keys, arguments, argument_length);

    if (result != nullptr) {
      result->incrementRefCount();
      if (object_init_ex(return_value, xdmMap_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmMap_object *vobj =
            (xdmMap_object *)((char *)vvobj - XtOffsetOf(xdmMap_object, std));
        assert(vobj != nullptr);
        vobj->xdmMap = result;
        return;
      }
    } else {
      RETURN_NULL();
    }
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, createXdmArrayValue) {
  SaxonProcessor *proc = nullptr;
  HashTable *arr_hash;

  zval *items_zval;
  zval *val;
  long num_key;
  XdmValue **arguments = nullptr;
  int argument_length = 0;
  zend_string *key;

  if (ZEND_NUM_ARGS() != 3) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &items_zval) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  SaxonProcessor *saxonProc = obj->saxonProcessor;
  arr_hash = Z_ARRVAL_P(items_zval);
  argument_length = zend_hash_num_elements(arr_hash);

  arguments = new XdmValue *[argument_length];

  if (saxonProc != nullptr) {
    ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {
      if (Z_TYPE_P(val) != IS_NULL) {
        const char *objName = ZSTR_VAL(Z_OBJCE_P(val)->name);

        if (strcmp(objName, "Saxon\\XdmNode") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmNode_object *nobj =
              (xdmNode_object *)((char *)ooth -
                                 XtOffsetOf(xdmNode_object, std));
          if (nobj != nullptr) {
            XdmValue *value = (XdmValue *)nobj->xdmNode;
            arguments[num_key] = value;
          }
        } else if (strcmp(objName, "Saxon\\XdmAtomicValue") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmAtomicValue_object *nobj =
              (xdmAtomicValue_object *)((char *)ooth -
                                        XtOffsetOf(xdmAtomicValue_object, std));
          if (nobj != nullptr) {
            XdmValue *value = (XdmValue *)nobj->xdmAtomicValue;
            arguments[num_key] = value;
          }
        } else if (strcmp(objName, "Saxon\\XdmValue") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xdmValue_object *nobj =
              (xdmValue_object *)((char *)ooth -
                                  XtOffsetOf(xdmValue_object, std));
          if (nobj != nullptr) {
            XdmValue *value = nobj->xdmValue;
            arguments[num_key] = value;
          }
        } else { // TODO error warning}
        }
      } else {
        // TODO error warning
      }
    }
    ZEND_HASH_FOREACH_END();

    XdmArray *result = saxonProc->makeArray(arguments, argument_length);

    if (result != nullptr) {
      result->incrementRefCount();
      if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmArray_object *vobj =
            (xdmArray_object *)((char *)vvobj -
                                XtOffsetOf(xdmArray_object, std));
        assert(vobj != nullptr);
        vobj->xdmArray = result;
        return;
      }
    } else {
      RETURN_NULL();
    }
  }
}

PHP_METHOD(SaxonProcessor, EQNameToClarkName) {
  SaxonProcessor *saxonProcessor;
  char *eqname;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &eqname, &len1) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    const char *qname = saxonProcessor->EQNameToClarkName(eqname);
    if (qname != NULL) {
      // char *str = estrdup(errStr);
      RETURN_STRING(qname);
    }
  }

  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, clarkNameToEQName) {
  SaxonProcessor *saxonProcessor;
  char *clname;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &clname, &len1) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    const char *qname = saxonProcessor->clarkNameToEQName(clname);
    if (qname != NULL) {
      // char *str = estrdup(errStr);
      RETVAL_STRING(qname);
      SaxonProcessor::deleteString(qname);
      return;
    }
  }

  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, createQNameValue) {
  SaxonProcessor *saxonProcessor;
  char *source;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    XdmAtomicValue *qname = saxonProcessor->makeQNameValue(source);
    if (qname != nullptr) {

      if (object_init_ex(return_value, xdmAtomicValue_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmAtomicValue_object *vobj =
            (xdmAtomicValue_object *)((char *)vvobj -
                                      XtOffsetOf(xdmAtomicValue_object, std));

        assert(vobj != nullptr);
        qname->incrementRefCount();
        vobj->xdmAtomicValue = qname;
        return;
      }
    } else {
    }
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, attachCurrentThread) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    saxonProcessor->attachCurrentThread();
    return;
  }
  return;
}
PHP_METHOD(SaxonProcessor, detachCurrentThread) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    saxonProcessor->detachCurrentThread();
    return;
  }
  return;
}

PHP_METHOD(SaxonProcessor, isSchemaAware) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    if (saxonProcessor->isSchemaAwareProcessor()) {
      RETURN_TRUE;
    }
  }
  RETURN_FALSE;
}

PHP_METHOD(SaxonProcessor, setCatalog) {
  SaxonProcessor *saxonProcessor;
  char *catalogFile;
  size_t len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &catalogFile, &len) ==
      FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    if (catalogFile != nullptr) {
      try {
        saxonProcessor->setCatalog(catalogFile);
      } catch (SaxonApiException &e) {
             zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    }
  }
}

PHP_METHOD(SaxonProcessor, setCatalogFiles) {
  SaxonProcessor *saxonProcessor;
  zval *items_zval;
  size_t len;
  HashTable *arr_hash;

  zval *val;
  long num_key;
  int argument_length = 0;
  zend_string *key;
  int i = 0;

  const char **catalogFiles = nullptr;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &items_zval) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    arr_hash = Z_ARRVAL_P(items_zval);
    argument_length = zend_hash_num_elements(arr_hash);

    catalogFiles = new const char *[argument_length];

    ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {
      if (Z_TYPE_P(val) == IS_STRING) {
        catalogFiles[i] = Z_STRVAL_P(val);

        i++;
      } else {
        // TODO error warning
      }
    }
    ZEND_HASH_FOREACH_END();

    if (catalogFiles != nullptr) {
      try {
        saxonProcessor->setCatalogFiles(catalogFiles, argument_length);
        delete[] catalogFiles;
      } catch (SaxonApiException &e) {
             zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    }
  }
}

PHP_METHOD(SaxonProcessor, setcwd) {
  SaxonProcessor *saxonProcessor;
  char *cwdStr;
  size_t len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &cwdStr, &len) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    if (cwdStr != nullptr) {
      saxonProcessor->setcwd(cwdStr);
    }
  }
}


PHP_METHOD(SaxonProcessor, getcwd) {
  SaxonProcessor *saxonProcessor;


  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
      const char *cwdStr = saxonProcessor->getcwd();
      if(cwdStr != nullptr) {
        RETURN_STRING(cwdStr);
      } else {
        RETURN_NULL();
      }
  }
}

PHP_METHOD(SaxonProcessor, parseJsonFromString) {
  SaxonProcessor *saxonProcessor;
  char *source;
  char *encoding = nullptr;
  size_t len1, len2;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 2 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &source, &len1,
                                   &encoding, &len2) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    XdmValue *value = saxonProcessor->parseJsonFromString(source, encoding);
    if (value != nullptr) {
      if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        value->incrementRefCount();
        zend_object *vobj = Z_OBJ_P(return_value);
        xdmValue_object *xdmVObj =
            (xdmValue_object *)((char *)vobj -
                                XtOffsetOf(xdmValue_object, std));
        assert(xdmVObj != nullptr);

        xdmVObj->xdmValue = value;
      }
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, parseJsonFromFile) {
  SaxonProcessor *saxonProcessor;
  char *source;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {

    XdmNode *node = nullptr;
    try {
        node = saxonProcessor->parseXmlFromFile(source);
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }

    if (node != nullptr) {

      if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmNode_object *vobj =
            (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));

        assert(vobj != nullptr);
        node->incrementRefCount();
        vobj->xdmNode = node;
        return;
      }
    } else {
    }
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, parseXmlFromString) {
  SaxonProcessor *saxonProcessor;
  SchemaValidator *validator = nullptr;
  zval *oth = nullptr;
  char *source;
  char *encoding = nullptr;
  size_t len1, len2;

  if (ZEND_NUM_ARGS() > 3) {
    WRONG_PARAM_COUNT;
  }

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 2 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &source, &len1,
                                   &oth) == FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 3 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "szs", &source, &len1, &oth,
                                   &encoding, &len2) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    if (oth != nullptr && Z_TYPE_P(oth) != IS_NULL) {
      zend_object *vvobj = Z_OBJ_P(oth);

      schemaValidator_object *ooth =
          (schemaValidator_object *)((char *)pobj -
                                     XtOffsetOf(schemaValidator_object, std));
      if (ooth != nullptr) {
        validator = ooth->schemaValidator;
      }
    }

    XdmNode *node = nullptr;
    try {
        node = saxonProcessor->parseXmlFromString(source, encoding, validator);
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    if (node != nullptr) {
      if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        node->incrementRefCount();
        zend_object *vobj = Z_OBJ_P(return_value);
        xdmNode_object *xdmNObj =
            (xdmNode_object *)((char *)vobj - XtOffsetOf(xdmNode_object, std));
        assert(xdmNObj != nullptr);

        xdmNObj->xdmNode = node;
      }
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, parseXmlFromFile) {
  SaxonProcessor *saxonProcessor;
  SchemaValidator *validator = nullptr;
  zval *oth = nullptr;
  char *source;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 2 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &source, &len1,
                                   &oth) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    if (oth != nullptr && Z_TYPE_P(oth) != IS_NULL) {
      zend_object *vvobj = Z_OBJ_P(oth);

      schemaValidator_object *ooth =
          (schemaValidator_object *)((char *)pobj -
                                     XtOffsetOf(schemaValidator_object, std));
      if (ooth != nullptr) {
        validator = ooth->schemaValidator;
      }
    }
    XdmNode *node = nullptr;

    try{
        node = saxonProcessor->parseXmlFromFile(source, validator);
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    if (node != nullptr) {

      if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmNode_object *vobj =
            (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));

        assert(vobj != nullptr);
        node->incrementRefCount();
        vobj->xdmNode = node;
        return;
      }
    } else {
    }
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, parseXmlFromUri) {
  SaxonProcessor *saxonProcessor;
  SchemaValidator *validator = nullptr;
  zval *oth = nullptr;
  char *source;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &source, &len1) == FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 2 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &source, &len1,
                                   &oth) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    if (oth != nullptr && Z_TYPE_P(oth) != IS_NULL) {
      zend_object *vvobj = Z_OBJ_P(oth);

      schemaValidator_object *ooth =
          (schemaValidator_object *)((char *)pobj -
                                     XtOffsetOf(schemaValidator_object, std));
      if (ooth != nullptr) {
        validator = ooth->schemaValidator;
      }
    }
    XdmNode *node = nullptr;
    try{
        node = saxonProcessor->parseXmlFromUri(source, validator);
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    if (node != nullptr) {

      if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmNode_object *vobj =
            (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));

        assert(vobj != nullptr);
        node->incrementRefCount();
        vobj->xdmNode = node;
        return;
      }
    } else {
    }
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, createAtomicValue) {
  XdmAtomicValue *xdmValue = nullptr;
  SaxonProcessor *proc = nullptr;
  char *source = nullptr;
  int len1 = 0;
  zval *zvalue;
  bool bVal = false;
  char *sVal = nullptr;
  int len = 0;
  long iVal = 0;
  double dVal;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &zvalue) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    switch (Z_TYPE_P(zvalue)) {
    case IS_FALSE:
      xdmValue = proc->makeBooleanValue(false);
    case IS_TRUE:
      xdmValue = proc->makeBooleanValue(true);
      break;
    case IS_LONG:
      iVal = Z_LVAL_P(zvalue);
      xdmValue = proc->makeIntegerValue((int)iVal);
      break;
    case IS_STRING:
      sVal = Z_STRVAL_P(zvalue);
      len = Z_STRLEN_P(zvalue);

      xdmValue = proc->makeStringValue((const char *)sVal);
      // efree(sVal);
      break;
    case IS_NULL:
      xdmValue = new XdmAtomicValue();
      break;
    case IS_DOUBLE:
      dVal = (double)Z_DVAL_P(zvalue);
      xdmValue = proc->makeDoubleValue((double)iVal);
      break;
    case IS_ARRAY:
      // TODO: Should not be expected. Do this some other way
      // break;
    case IS_OBJECT:
      // TODO: implement this
      // break;
    default:
      obj = nullptr;
      zend_throw_exception(zend_ce_exception,
                           "unknown type specified in XdmValue", 0);
      RETURN_NULL();
    }
    if (xdmValue == nullptr) {
      RETURN_NULL();
    }
    if (object_init_ex(return_value, xdmAtomicValue_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      // struct xdmAtomicValue_object* vobj = (struct xdmAtomicValue_object
      // *)Z_OBJ_P(return_value);
      zend_object *vvobj = Z_OBJ_P(return_value);
      xdmAtomicValue_object *vobj =
          (xdmAtomicValue_object *)((char *)vvobj -
                                    XtOffsetOf(xdmAtomicValue_object, std));
      assert(vobj != nullptr);
      xdmValue->incrementRefCount();
      vobj->xdmAtomicValue = xdmValue;
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newDocumentBuilder) {
  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  DocumentBuilder *documentBuilder = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, documentBuilder_ce) != SUCCESS) {
      RETURN_NULL();
    } else {

      zend_object *vvobj = Z_OBJ_P(return_value);
      documentBuilder_object *vobj =
          (documentBuilder_object *)((char *)vvobj -
                                     XtOffsetOf(documentBuilder_object, std));
      assert(vobj != nullptr);
      documentBuilder = proc->newDocumentBuilder();
      if (documentBuilder == nullptr) {
        RETURN_NULL();
      }
      vobj->documentBuilder = documentBuilder;
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newXPathProcessor) {
  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  XPathProcessor *xpathProcessor = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, xpathProcessor_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      // struct xpathProcessor_object* vobji = (struct xpathProcessor_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      zend_object *vvobj = Z_OBJ_P(return_value);
      xpathProcessor_object *vobj =
          (xpathProcessor_object *)((char *)vvobj -
                                    XtOffsetOf(xpathProcessor_object, std));
      assert(vobj != nullptr);
      xpathProcessor = proc->newXPathProcessor();
      if (xpathProcessor == nullptr) {
        RETURN_NULL();
      }
      vobj->xpathProcessor = xpathProcessor;
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newXslt30Processor) {
  // std::cerr<<"SaxonProcessor newXslt30proc point 1"<<std::endl;
  //  php_error(E_WARNING,"new xslt30Proc 1");
  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  Xslt30Processor *xslt30Processor = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  // saxonProcessor_object *obj = (saxonProcessor_object *)Z_OBJ_P(getThis());
  assert(obj != nullptr);
  proc = obj->saxonProcessor;

  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, xslt30Processor_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      xslt30Processor = proc->newXslt30Processor();
      if (xslt30Processor == nullptr) {
        RETURN_NULL();
      }
      zend_object *vobj = Z_OBJ_P(return_value);
      xslt30Processor_object *xproc_object =
          (xslt30Processor_object *)((char *)vobj -
                                     XtOffsetOf(xslt30Processor_object, std));
      assert(vobj != nullptr);

      xproc_object->xslt30Processor = xslt30Processor;
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newXQueryProcessor) {

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  XQueryProcessor *xqueryProcessor = nullptr;
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, xqueryProcessor_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      xqueryProcessor = proc->newXQueryProcessor();
      zend_object *vvobj = Z_OBJ_P(return_value);
      xqueryProcessor_object *vobj =
          (xqueryProcessor_object *)((char *)vvobj -
                                     XtOffsetOf(xqueryProcessor_object, std));
      assert(vvobj != nullptr);

      vobj->xqueryProcessor = xqueryProcessor;
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newSchemaValidator) {

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  SchemaValidator *schemaValidator = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, schemaValidator_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      // struct schemaValidator_object* vobj = (struct schemaValidator_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      zend_object *vvobj = Z_OBJ_P(return_value);
      schemaValidator_object *vobj =
          (schemaValidator_object *)((char *)vvobj -
                                     XtOffsetOf(schemaValidator_object, std));
      assert(vobj != nullptr);
      try {
        schemaValidator = proc->newSchemaValidator();
        if (schemaValidator == nullptr) {
            RETURN_NULL();
        }
        vobj->schemaValidator = schemaValidator;
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, newXsdCompiler) {

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  SaxonProcessor *proc;
  XsdCompiler *compiler = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  assert(obj != nullptr);
  proc = obj->saxonProcessor;
  assert(proc != nullptr);
  if (proc != nullptr) {
    if (object_init_ex(return_value, xsdCompiler_ce) != SUCCESS) {
      RETURN_NULL();
    } else {
      // struct schemaValidator_object* vobj = (struct schemaValidator_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      zend_object *vvobj = Z_OBJ_P(return_value);
      xsdCompiler_object *vobj =
          (xsdCompiler_object *)((char *)vvobj -
                                     XtOffsetOf(xsdCompiler_object, std));
      assert(vobj != nullptr);
      try {
        compiler = proc->newXsdCompiler();
        if (compiler == nullptr) {
            RETURN_NULL();
        }
        vobj->xsdCompiler = compiler;
      } catch (SaxonApiException &e) {
         zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
      }
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(SaxonProcessor, getSaxonEdition) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    const char *editionStr = saxonProcessor->getSaxonEdition();
    RETURN_STRING(editionStr);
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, isLicensed) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    bool flag = saxonProcessor->isLicensed();
    RETURN_BOOL(flag);
  }
  RETURN_BOOL(false);
}

PHP_METHOD(SaxonProcessor, version) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));
  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    const char *verStr = saxonProcessor->version();
    RETURN_STRING(verStr);
  }
  RETURN_NULL();
}

PHP_METHOD(SaxonProcessor, setConfigurationProperty) {
  SaxonProcessor *saxonProcessor;
  char *name;
  char *value;
  size_t len1, len2;
  if (ZEND_NUM_ARGS() != 2) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &name, &len1, &value,
                            &len2) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr && name != nullptr && value != nullptr) {
    saxonProcessor->setConfigurationProperty(name, value);
  }
}

PHP_METHOD(SaxonProcessor, clearConfigurationProperties) {
  SaxonProcessor *saxonProcessor;

  if (ZEND_NUM_ARGS() != 0) {
    WRONG_PARAM_COUNT;
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  saxonProcessor_object *obj =
      (saxonProcessor_object *)((char *)pobj -
                                XtOffsetOf(saxonProcessor_object, std));

  saxonProcessor = obj->saxonProcessor;
  if (saxonProcessor != nullptr) {
    saxonProcessor->clearConfigurationProperties();
  }
}

typedef enum saxonTypeEnum {
  enumNode,
  enumString,
  enumInteger,
  enumDouble,
  enumFloat,
  enumBool,
  enumArrXdmValue
} PRIMITIVE_TYPE;

zval makePHPValue(void * value){

  zval retval;
//TODO move this hashmap somewhere else to prevent repeatable construction
  std::map<std::string, saxonTypeEnum> typeMap;
  typeMap["node"] = enumNode;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}string"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}duration"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}dateTime"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}date"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}gYearMonth"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}gYear"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}gMonthDay"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}time"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}integer"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}double"] = enumDouble;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}decimal"] = enumDouble;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}float"] = enumFloat;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}boolean"] = enumBool;

  typeMap["Q{http://www.w3.org/2001/XMLSchema}gDay"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}gMonth"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}hexBinary"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}base64Binary"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}anyURI"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}QName"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}NOTATION"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}numeric"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}nonPositiveInteger"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}negativeInteger"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}long"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}int"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}short"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}byte"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}nonNegativeInteger"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}positiveInteger"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}unsignedLong"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}unsignedInt"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}unsignedShort"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}unsignedByte"] = enumInteger;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}normalizedString"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}token"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}anyType"] = enumString;
  typeMap["Q{http://www.w3.org/2001/XMLSchema}anySimpleType"] = enumString;

  typeMap["[xdmvalue"] = enumArrXdmValue;

  int typeRef = j_getXdmObjectType(SaxonProcessor::sxn_environ->thread, value);

  allocFn fn = operator new;

  XDM_TYPE xdm_type = static_cast<XDM_TYPE>(typeRef);

  if (getenv("SAXONC_DEBUG_FLAG")) {
    std::cerr<<"Xdm typeRef = "<< typeRef<<std::endl;
  }

  if (typeRef == SXN_EXCEPTION) {

    //throw SaxonApiException();

    zend_throw_exception(zend_ce_exception, "Error found in converting arguments", 0);
  }

  if (xdm_type == XDM_ATOMIC_VALUE) {

    graal_isolatethread_t *thread = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
    char * primType = j_getPrimitiveTypeName(thread,(void *)fn, (void *)value);

    //XdmAtomicValue * aitem = (XdmAtomicValue *) (value->getHead());
    //const char * primType = aitem->getPrimitiveTypeName();

    if (getenv("SAXONC_DEBUG_FLAG")) {
      std::cerr<<"Primitive type = "<< primType<<std::endl;
    }

    saxonTypeEnum typeValue;

    if(primType == NULL) {
        typeValue = typeMap["Q{http://www.w3.org/2001/XMLSchema}anySimpleType"];
    } else {
        typeValue = typeMap[primType];
        operator delete(primType);
    }

    bool bvalue;
    long lnumber;
    double dvalue;
    const char * astri = nullptr;
    saxon_error_t r = SAXON_ERROR_DEFAULT;

    switch (typeValue) {
      case saxonTypeEnum::enumBool:
      bvalue = j_getBooleanValue(thread, (void *)value, &r) != 0;
      ZVAL_BOOL(&retval, bvalue);
      break;
      case saxonTypeEnum::enumInteger:
      lnumber = j_getLongValue(thread, (void *)value, &r);
      ZVAL_LONG(&retval, lnumber);
      break;
      case saxonTypeEnum::enumString:
        astri = (const char *)getStringValueForXdmItem(thread,(void *)fn, (void *)value, NULL,  &r);//aitem->getStringValue();
      ZVAL_STRING(&retval, astri);
      break;
      case saxonTypeEnum::enumDouble:
      case saxonTypeEnum::enumFloat:
        dvalue = j_getDoubleValue(thread, (void *)value, &r);//aitem->getDoubleValue();
      ZVAL_DOUBLE(&retval, dvalue);
      break;

      default:
        if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"PHP internal: type not found"<< std::endl;
        }
        astri = (const char *)getStringValueForXdmItem(thread,(void *)fn, (void *)value, NULL, &r);//aitem->getStringValue();
        ZVAL_STRING(&retval, astri);
        break;

    }
    if (r.status != SAXON_OK) {
       zend_throw_exception(zend_ce_exception, r.message, 0);
    }

  	return retval;
} else if (xdm_type == XDM_NODE) {
  int64_t copiedValue = (int64_t)j_copy_handles(SaxonProcessor::sxn_environ->thread, value);
  XdmNode * nxdmItem = new XdmNode(copiedValue);
  //XdmNode * nxdmItem = (XdmNode *)(value->getHead());
  if (object_init_ex(&retval, xdmNode_ce) != SUCCESS) {
    //error

    php_error(E_WARNING,"error phpNative xdmNode creation failed");
  }

  struct xdmNode_object* vobj;
  zend_object* zend_vobj;

  //MAKE_STD_ZVAL(php_argv[i]._val);
  zend_vobj = Z_OBJ_P(&retval);
  vobj = (xdmNode_object *)((char *)zend_vobj - XtOffsetOf(xdmNode_object, std));
  assert (vobj != NULL);

  vobj->xdmNode = nxdmItem;
  nxdmItem->incrementRefCount(); //TODO maybe we should be copying values
  //delete value;
  return retval;


}  else if (xdm_type == XDM_MAP) {

   graal_isolatethread_t *thread = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
  int count = j_xdmMap_size(thread, (void *)value);
  //int count = mitem->mapSize();

  if (getenv("SAXONC_DEBUG_FLAG")) {
    std::cerr<<"PHPValue: XDMMap size="<<count<<std::endl;
  }

  ///// creating PHP array
  zend_array *ret = zend_new_array(count);
  zend_ulong idx = 0;
  saxon_error_t r = SAXON_ERROR_DEFAULT;

  //graal_isolatethread_t *thread = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
  long long *rawKeys = j_xdmMap_keys(thread,(void *)fn, (void *)value);

  long long * rawMapValues = j_xdmMap_values(thread,(void *)fn, (void *)value, &r);

    if (r.status != SAXON_OK) {
       zend_throw_exception(zend_ce_exception, r.message, 0);
    }

  char * ss = nullptr;

    if (getenv("SAXONC_DEBUG_FLAG")) {
      std::cerr<<"PHPValue: XDMMap before loop call"<<std::endl;
    }

  for (int j = 0; j < count; j++) {
    //XdmValue * avaluej =mitem->get(aKeys[j]);
    //todo add encoding instead of NULL
    ss = getStringValueForXdmItem(thread,(void *)fn, (void *)rawKeys[j], NULL, &r);
    //const char * ss = aKeys[j]->getStringValue();
  if (getenv("SAXONC_DEBUG_FLAG")) {
    std::cerr<<"PHPValue: XDMMap key str="<<ss<<std::endl;
  }
    zend_string * keyzvalj = zend_string_init(ss, strlen(ss), 0);


    zval retvalj = makePHPValue((void *)rawMapValues[j]);

    zend_hash_add(ret, keyzvalj, &retvalj);
    //delete avaluej;

    operator delete(ss);

  }
  //delete [] aKeys;
  ZVAL_ARR(&retval, ret);

  for(int j = 0; j < count; j++) {
    j_handles_destroy(thread, (void *)rawKeys[j]);
    j_handles_destroy(thread, (void *)rawMapValues[j]);
  }
  operator delete(rawKeys);
  operator delete(rawMapValues);
  /*delete mitem;
  delete value;*/

  //value->resetRelinquishedItems();
  //delete value;

  //TODO XdmMap objectHandle pool not getting cleared.. Clear Keys and values

  return retval;

  } else if (xdm_type == XDM_ARRAY) {

  graal_isolatethread_t *thread = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
  int count = j_xdmArray_arrayLength(thread,(void *)value);

  if (getenv("SAXONC_DEBUG_FLAG")) {
    std::cerr<<"PHPValue: XDMarray size="<<count<<std::endl;
  }

  saxon_error_t r = SAXON_ERROR_DEFAULT;

  ///// creating PHP array
  zend_array *ret = zend_new_array(count);
  zend_ulong idx = 0;
  int64_t avaluej =-1;
  for (int j = 0; j < count; j++) {

    avaluej =
        j_xdmArray_get(thread, (void *)value, j, &r);

    if (r.status != SAXON_OK) {
        zend_throw_exception(zend_ce_exception, r.message, 0);
    }

    zval retvali = makePHPValue((void *)avaluej);

    j_handles_destroy(thread, (void *)avaluej);

    zend_hash_index_add(ret, idx, &retvali);

    idx++;
  }
  ZVAL_ARR(&retval, ret);
  //value->resetRelinquishedItems();
  //delete value;
  return retval;


} else if (xdm_type == XDM_FUNCTION_ITEM) {
    int64_t copiedValue = (int64_t)j_copy_handles(SaxonProcessor::sxn_environ->thread, value);
	XdmFunctionItem * funcxdmItem = new XdmFunctionItem((int64_t)copiedValue);

          struct xdmFunctionItem_object* aavobj;
          zend_object* zend_avobj;
		  funcxdmItem->incrementRefCount();
          if (object_init_ex(&retval, xdmFunctionItem_ce) != SUCCESS) {
            //error

            zend_throw_exception(zend_ce_exception, "Error found in creating XdmFunctionItem", 0);
          }

          zend_avobj = Z_OBJ_P(&retval);
          aavobj = (xdmFunctionItem_object *)((char *)zend_avobj - XtOffsetOf(xdmFunctionItem_object, std));
          assert (aavobj != NULL);

          aavobj->xdmFunctionItem = funcxdmItem;


} else if (xdm_type == XDM_EMPTY) {
  //delete value;
}else if (xdm_type == XDM_VALUE) {

  graal_isolatethread_t *thread = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
  long long *rawValues = makeArrayFromXdmValue2(thread, (void *)fn, (void *)value);

  int count = (int) rawValues[0];
  if (getenv("SAXONC_DEBUG_FLAG")) {
    std::cerr<<"PHPValue: XDMValue size="<<count<<std::endl;
  }

  ///// creating PHP array
  zend_array *ret = zend_new_array(count);
  zend_ulong idx = 0;

  for (int j = 1; j <= count; j++) {
    //XdmItem *itemi = value->itemAt(j);
    //itemi->incrementRefCount();

    zval retvali = makePHPValue((void *) rawValues[j]);

    zend_hash_index_add(ret, idx, &retvali);

    idx++;
  }

  zval arrval;
  ZVAL_ARR(&arrval, ret);

    for(int j = 1; j <= count; j++) {
      j_handles_destroy(thread, (void *)rawValues[j]);

    }

    operator delete(rawValues);


  //delete value;
    ///// create PHP ArrayIterator
  if (object_init_ex(&retval,spl_ce_ArrayIterator) != SUCCESS) {

  } else {

   zend_call_method_with_1_params(Z_OBJ(retval), spl_ce_ArrayIterator, &spl_ce_ArrayIterator->constructor, "__construct", NULL, &arrval);

   if (getenv("SAXONC_DEBUG_FLAG")) {
      std::cerr<<"PHP internal: makePHPValue ArrayIterator called"<< std::endl;
    }
  }


  return retval;

}
  zend_throw_exception(zend_ce_exception, "Internal Error: Cannot convert XdmValue to PHP value", 0);
  zval result_null;
  ZVAL_NULL(&result_null);
  return result_null;
}


void * makeXdmValueFromPHPZVal(SaxonProcessor * nprocessor, zval * retval){

       graal_isolatethread_t *thread1 = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
              if (getenv("SAXONC_DEBUG_FLAG")) {
                std::cerr<<"makeXdmValueFromPHPZVal cp0"<< std::endl;
              }
       if(retval == nullptr || Z_TYPE_P(retval) ==0){
            //zend_error(E_ERROR, "Function returned null");
              if (getenv("SAXONC_DEBUG_FLAG")) {
                std::cerr<<"PHP internal: Z_TYPE_P(retval)~~"<< std::endl;
              }
            zend_throw_exception(zend_ce_exception, "Function returned null", 0);
            return NULL;
        }

        XdmValue * resultValue = nullptr;

        void * resultVoidValue = nullptr;

        const char * objName = NULL;
        xdmNode_object* nooth = NULL;
        xdmAtomicValue_object* aooth = NULL;
        xdmValue_object* vooth = NULL;
        xdmArray_object* arrooth = NULL;
        xdmMap_object* mooth = NULL;
        xdmFunctionItem_object* f_ooth = NULL;

        char * sVal = NULL;
        int len = 0;

        zend_object* zend_vobj2;

        if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"PHP internal: makeXdmValueFromPHPZVal returnType value = "<<((int)(Z_TYPE_P(retval)))<< std::endl;

        }


        HashTable *rv_arr_hash;
        int rv_arr_length = 0;
        zend_string *rv_key;
  		zval *arr_val;
  		long num_key;
        void ** arrItems = nullptr;

        zval *arrIter_retval = nullptr;
        zval *arrIter_valid = nullptr;
        void * valuei = nullptr;
        zval tmp;

       saxon_error_t r = SAXON_ERROR_DEFAULT;

        switch (Z_TYPE_P(retval)) {
            case IS_FALSE:
                return (void *)j_makeBooleanValue(thread1, (int)false);
            case IS_TRUE:
                return (void *)j_makeBooleanValue(thread1, (int)true);

            case IS_LONG:
                //resultValue = nprocessor->makeIntegerValue(Z_LVAL_P(retval));
                return (void *)j_makeLongValue(thread1, Z_LVAL_P(retval));

            case IS_STRING:
                sVal = Z_STRVAL_P(retval);
                len = Z_STRLEN_P(retval);
                //resultValue = nprocessor->makeStringValue(estrndup(sVal,len));
                valuei = (void *)j_makeStringValue(thread1, (char *)estrndup(sVal,len), (char *)NULL, &r);
                if (r.status != SAXON_OK) {
                    zend_throw_exception(zend_ce_exception, r.message, 0);
                }
                return valuei;
            case IS_NULL:
              if (getenv("SAXONC_DEBUG_FLAG")) {
                std::cerr<<"PHP internal: call_user_function return is NULL"<< std::endl;
              }
                return nullptr;
            case IS_DOUBLE:
                //resultValue = nprocessor->makeDoubleValue((double)Z_DVAL_P(retval));
                return (void *)j_makeDoubleValue(thread1, (double)Z_DVAL_P(retval));

            case IS_ARRAY:
              if (getenv("SAXONC_DEBUG_FLAG")) {
                std::cerr<<"PHP internal: call_user_function return is an array"<< std::endl;
              }
              rv_arr_hash = Z_ARRVAL_P(retval);
      		  rv_arr_length = zend_hash_num_elements(rv_arr_hash);

			  resultVoidValue = (void *)createProcessorDataWithCapacity(thread1, rv_arr_length);

              //arrItems = new void*[rv_arr_length]; // TODO delete pointer array when finished

              ZEND_HASH_FOREACH_KEY_VAL(rv_arr_hash, num_key, rv_key, arr_val) {
                	valuei = makeXdmValueFromPHPZVal(nprocessor, arr_val);
                    addProcessorValue(thread1, resultVoidValue,valuei);
                    j_handles_destroy(thread1, valuei);
              }
              ZEND_HASH_FOREACH_END();
              //return SaxonProcessor::makeInternalArray(arrItems, rv_arr_length);
              valuei = (void *) j_makeXdmArray(thread1, (void *)resultVoidValue, &r);
              if (r.status != SAXON_OK) {
                  zend_throw_exception(zend_ce_exception, r.message, 0);
              }
              j_handles_destroy(thread1, (void *)resultVoidValue);
              return valuei;

            case IS_OBJECT:
              if (getenv("SAXONC_DEBUG_FLAG")) {
                std::cerr<<"PHP internal: call_user_function return is an object"<< std::endl;
              }
               if(/*Z_OBJCE_P(retval) != nullptr && */instanceof_function(Z_OBJCE_P(retval), spl_ce_ArrayIterator)) {
				 if (!arrIter_valid) {
					ZVAL_UNDEF(&tmp);
					arrIter_valid = &tmp;
				 }
			  	 zend_call_method_with_0_params(Z_OBJ_P(retval), spl_ce_ArrayIterator, NULL, "valid", arrIter_valid);
                if(Z_TYPE_P(arrIter_valid) == IS_NULL) {
                      std::cerr<<"PHP internal: call_user_function return arrIter_valid is NULL"<< std::endl;
                    }
					if (getenv("SAXONC_DEBUG_FLAG")) {
          				std::cerr<<"PHP internal: makeXdmValueFromPHPZVal returnType arrIter_valid = "<<((int)(Z_TYPE_P(arrIter_valid)))<< std::endl;

        			}

                 if(Z_TYPE_P(arrIter_valid) == IS_TRUE) {
                   if (getenv("SAXONC_DEBUG_FLAG")) {
                    std::cerr<<"PHP internal: call_user_function return is an ArrayIterator - converting to sequence"<< std::endl;
                 	}
                   //resultValue = new XdmValue();

    				resultVoidValue  = (void *)createProcessorData(thread1);
                 	while(Z_TYPE_P(arrIter_valid) == IS_TRUE) {
					 	if (!arrIter_retval) {
							ZVAL_UNDEF(&tmp);
							arrIter_retval = &tmp;
				 		}
						zend_call_method_with_0_params(Z_OBJ_P(retval), spl_ce_ArrayIterator, NULL, "current", arrIter_retval);
						if (getenv("SAXONC_DEBUG_FLAG")) {
                    		std::cerr<<"PHP internal: call_user_function return is an ArrayIterator.current cp1"<< std::endl;
                 		}
						valuei = makeXdmValueFromPHPZVal(nprocessor, arrIter_retval);
                    	if(valuei != nullptr) {
                          //TODO The XdmItems in the resultValue are not getting freed and the OBject Handles pool is also not getting cleared
                    		//resultValue->addUnderlyingValue((int64_t)valuei);

							addProcessorValue(thread1, resultVoidValue,valuei);
							j_handles_destroy(thread1, valuei);
                    	} else {
                      	//do nothing
                    	}
                        zend_call_method_with_0_params(Z_OBJ_P(retval), spl_ce_ArrayIterator, NULL, "next", NULL);
                        zend_call_method_with_0_params(Z_OBJ_P(retval), spl_ce_ArrayIterator, NULL, "valid", arrIter_valid);
                 	}
                 }

                 return resultVoidValue;
               } else {

                objName =ZSTR_VAL(Z_OBJCE_P(retval)->name);

                if(objName == NULL) {
                  zend_throw_exception(zend_ce_exception, "Unknown type specified in extension function", 0);
                }

                if (getenv("SAXONC_DEBUG_FLAG")) {
                  std::cerr<<"PHP internal: call_user_function returnType obj = "<<objName<< std::endl;
                }

                if(strcmp(objName, "Saxon\\XdmNode")==0) {

                    zend_vobj2 =  Z_OBJ_P(retval);
                    nooth = (xdmNode_object *)((char *)zend_vobj2 -
                                              XtOffsetOf(xdmNode_object, std));

                    if(nooth != NULL) {
                        resultValue = nooth->xdmNode;
                    }
                } else if(strcmp(objName, "Saxon\\XdmAtomicValue")==0) {

                  zend_vobj2 =  Z_OBJ_P(retval);
                  aooth = (xdmAtomicValue_object *)((char *)zend_vobj2 -
                                            XtOffsetOf(xdmAtomicValue_object, std));

                  if(aooth != NULL) {
                    resultValue = aooth->xdmAtomicValue;
                  }
                } else if(strcmp(objName, "Saxon\\XdmMap")==0) {

                  zend_vobj2 =  Z_OBJ_P(retval);
                  mooth = (xdmMap_object *)((char *)zend_vobj2 -
                                            XtOffsetOf(xdmMap_object, std));

                  if(mooth != NULL) {
                    resultValue = mooth->xdmMap;
                  }
                } else if(strcmp(objName, "Saxon\\XdmArray")==0) {

                  zend_vobj2 =  Z_OBJ_P(retval);
                  arrooth = (xdmArray_object *)((char *)zend_vobj2 -
                                            XtOffsetOf(xdmArray_object, std));

                  if(arrooth != NULL) {
                    resultValue = arrooth->xdmArray;
                  }
                } else if(strcmp(objName, "Saxon\\XdmFunctionItem")==0) {

                  zend_vobj2 =  Z_OBJ_P(retval);
                  f_ooth = (xdmFunctionItem_object *)((char *)zend_vobj2 -
                                            XtOffsetOf(xdmFunctionItem_object, std));

                  if(f_ooth != NULL) {
                    resultValue = f_ooth->xdmFunctionItem;
                  }
                } else if(strcmp(objName, "Saxon\\XdmValue")==0) {

                  zend_vobj2 =  Z_OBJ_P(retval);
                  vooth = (xdmValue_object *)((char *)zend_vobj2 -
                                            XtOffsetOf(xdmValue_object, std));

                  if(vooth != NULL) {
                    resultValue = vooth->xdmValue;
                  }
                }
                }
                break;
            case IS_UNDEF:
                zend_throw_exception(zend_ce_exception, "User extension function throw an extension", 0);

            default:
                zend_throw_exception(zend_ce_exception, "Unknown type specified in extension function", 0);
        }

      if(resultValue == nullptr){
        zend_throw_exception(zend_ce_exception, "Error in converting PHP return type to XdmValue from user extension function", 0);
      }
      if (getenv("SAXONC_DEBUG_FLAG")) {
      	std::cerr<<"makeXdmValueFromPHP return.underlying ref= "<<resultValue->getUnderlyingValue()<<std::endl;
      }

      resultVoidValue = (void *)resultValue->getUnderlyingValue();
      int64_t resultCopiedVoidValue = j_copy_handles(thread1, resultVoidValue);
      //TODO check for memory leak - if we need to delete the resultValue.
      //delete resultValue;


      return (void *)resultCopiedVoidValue;
}


void * userFunctionPHPNativeCall(char * functionName, void * proc, void * func, int useNativeTypes, int numOfArguments,  void ** argumentsi)
{
    if(functionName == NULL) {
        if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"Error: phpNativeCall - functionName is null"<<std::endl;
        }
        return NULL;
    }

    graal_isolatethread_t *thread1 = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
    std::string nativeString2 = std::string(functionName);


 zval function_name;
 zval retval;

    int argLength = 0;
    zval  *  params;
    if(argumentsi != NULL) {
        argLength = numOfArguments;

        params =  new zval[argLength];
    } else {
        params = NULL;
    }

    sxnc_value* sresult = (sxnc_value *)malloc(sizeof(sxnc_value));


    if(useNativeTypes) {
        if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"DEBUG: phpNativeCall - useNativeTypes"<<std::endl;
        }
        for(int i=0; i<argLength;i++){
          params[i] = makePHPValue(argumentsi[i]);
        }

    } else {

        XdmValue ** arguments = NULL;
        int64_t argument = -1;
        if(numOfArguments > 0) {
            arguments = new XdmValue*[numOfArguments];
            for(int i =0; i< numOfArguments; i++) {
                if (getenv("SAXONC_DEBUG_FLAG")) {
                    fprintf(stdout, "Making XdmValue ref=%ld.\n", (long)argumentsi[i]);
                }
                argument = j_copy_handles(thread1, (void *)argumentsi[i]);
                arguments[i] = SaxonProcessor::makeXdmValueFromRef(argument);
            }
        }

      if (getenv("SAXONC_DEBUG_FLAG")) {
        std::cerr<<"DEBUG: phpNativeCall - using Xdmtypes"<<std::endl;
      }

      zval * oth = nullptr;
      for(int i=0; i<argLength;i++){
        int typeRef = j_getXdmObjectType(thread1,
                                 (void *)(arguments[i])->getUnderlyingValue());

        XDM_TYPE xdm_type = static_cast<XDM_TYPE>(typeRef);

        if (typeRef == SXN_EXCEPTION) {

          //throw SaxonApiException();

          zend_throw_exception(zend_ce_exception, "Error found in converting arguments", 0);
        }

        if (xdm_type == XDM_ATOMIC_VALUE) {
          XdmAtomicValue * axdmItem = (XdmAtomicValue *)((arguments[i])->getHead());

          if (object_init_ex(&params[i], xdmAtomicValue_ce) != SUCCESS) {
            return nullptr;
          }

		  axdmItem->incrementRefCount();
          zend_object *zavobj = Z_OBJ_P(&params[i]);
          xdmAtomicValue_object *avobj =
              (xdmAtomicValue_object *)((char *)zavobj -
                                        XtOffsetOf(xdmAtomicValue_object, std));

          assert(avobj != nullptr);
                                                                                                                                                                                                                                                                                                                                              avobj->xdmAtomicValue = axdmItem;

          continue;


        } else if (xdm_type == XDM_NODE) {
          XdmNode * nodeItem = (XdmNode *)((arguments[i])->getHead());

          if (object_init_ex(&params[i], xdmNode_ce) != SUCCESS) {
            return nullptr;
          }
		  nodeItem->incrementRefCount();
          zend_object *znvobj = Z_OBJ_P(&params[i]);
          xdmNode_object *nvobj =
              (xdmNode_object *)((char *)znvobj -
                                        XtOffsetOf(xdmNode_object, std));

          assert(nvobj != nullptr);
          nvobj->xdmNode = nodeItem;

        } else if (xdm_type == XDM_MAP) {

          XdmMap * mapxdmItem = (XdmMap *)((arguments[i])->getHead());

          struct xdmMap_object* aavobj;
          zend_object* zend_avobj;
		  mapxdmItem->incrementRefCount();
          if (object_init_ex(&params[i], xdmArray_ce) != SUCCESS) {
            //error

            zend_throw_exception(zend_ce_exception, "Error found in converting user function argument to XdmMap", 0);
          }

          zend_avobj = Z_OBJ_P(&params[i]);
          aavobj = (xdmMap_object *)((char *)zend_avobj - XtOffsetOf(xdmMap_object, std));
          assert (aavobj != NULL);

          aavobj->xdmMap = mapxdmItem;


        } else if (xdm_type == XDM_ARRAY) {

          XdmArray * arrxdmItem = (XdmArray *)((arguments[i])->getHead());

          struct xdmArray_object* aavobj;
          zend_object* zend_avobj;
		  arrxdmItem->incrementRefCount();
          if (object_init_ex(&params[i], xdmArray_ce) != SUCCESS) {
            //error

            zend_throw_exception(zend_ce_exception, "Error found in converting user function argument to XdmArray", 0);
          }

          zend_avobj = Z_OBJ_P(&params[i]);
          aavobj = (xdmArray_object *)((char *)zend_avobj - XtOffsetOf(xdmArray_object, std));
          assert (aavobj != NULL);

          aavobj->xdmArray = arrxdmItem;


        } else if (xdm_type == XDM_FUNCTION_ITEM) {
          XdmFunctionItem * funcxdmItem = (XdmFunctionItem *)((arguments[i])->getHead());

          struct xdmFunctionItem_object* aavobj;
          zend_object* zend_avobj;
		  funcxdmItem->incrementRefCount();
          if (object_init_ex(&params[i], xdmFunctionItem_ce) != SUCCESS) {
            //error

            zend_throw_exception(zend_ce_exception, "Error found in converting user function argument to XdmFunctionItem", 0);
          }

          zend_avobj = Z_OBJ_P(&params[i]);
          aavobj = (xdmFunctionItem_object *)((char *)zend_avobj - XtOffsetOf(xdmFunctionItem_object, std));
          assert (aavobj != NULL);

          aavobj->xdmFunctionItem = funcxdmItem;

        } else if (xdm_type == XDM_EMPTY) {
          // Empty Sequence detected.


          //return nullptr;

        } else if (xdm_type == XDM_VALUE) {
          XdmValue * valuei = (XdmValue *)((arguments[i]));

          struct xdmValue_object* vvobj;
          zend_object* zend_vvobj;

          if (object_init_ex(&params[i], xdmValue_ce) != SUCCESS) {
            //error

            zend_throw_exception(zend_ce_exception, "Error found in converting user function argument to XdmValue", 0);
          }
		  valuei->incrementRefCount();
          zend_vvobj = Z_OBJ_P(&params[i]);
          vvobj = (xdmValue_object *)((char *)zend_vvobj - XtOffsetOf(xdmValue_object, std));
          assert (vvobj != NULL);

          vvobj->xdmValue = valuei;

        } else {
          zend_throw_exception(zend_ce_exception, "Internal Error: Cannot create XdmItem from reference", 0);
        }
      }


     /*for(int i =0; i< numOfArguments; i++) {
            delete arguments1[i];
     } */
     delete [] arguments;
     arguments = nullptr;
    }

    SaxonProcessor * nprocessor = nullptr;

    if(proc == nullptr) {
        nprocessor = new SaxonProcessor(true); //processor object created for XdmNode
    }  else {
        nprocessor = (SaxonProcessor*) proc;
    }

    zval* callOnObj = NULL;

    //TODO error tests on function names
    ZVAL_STRING(&function_name, nativeString2.c_str());
    if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"PHP internal: call_user_function on = "<<nativeString2<< std::endl;
    }

    if(call_user_function(CG(function_table), (zval*)callOnObj,
                                 &function_name, &retval, argLength, params) != SUCCESS){
       zend_throw_exception(zend_ce_exception, "Function call failed", 0);
       if (getenv("SAXONC_DEBUG_FLAG")) {
          std::cerr<<"PHP internal: call_user_function FAILED!!!"<< std::endl;
       }
       return nullptr;
    }

    void * resultValue = makeXdmValueFromPHPZVal(nprocessor, &retval);

    zval_dtor(&retval);
    zval_dtor(&function_name);
    if(resultValue == nullptr){
      return nullptr;
    }
    //void* resultPointer = j_copy_handles(thread1, resultValue);

    return resultValue;
}


PHP_METHOD(SaxonProcessor, registerUserFunction)
{

    SaxonProcessor *saxonProcessor;
    char * namespacei = nullptr;
    char * signature = nullptr;
    bool useNativeTypes = true;
    size_t len1, len2;

    if (zend_parse_parameters(ZEND_NUM_ARGS() , "ssb", &namespacei, &len1, &signature, &len2, &useNativeTypes) ==
FAILURE) { RETURN_NULL();
    }

    zend_object* pobj = Z_OBJ_P(getThis());
    saxonProcessor_object * obj = (saxonProcessor_object *)((char *)pobj -
XtOffsetOf(saxonProcessor_object, std));

  if(signature == nullptr) {
       zend_throw_exception(zend_ce_exception, "Method signature is null", 0);
   }

    saxonProcessor = obj->saxonProcessor;
    graal_isolatethread_t *thread1 = SaxonProcessor::getOrAttachCurrentThread(SaxonProcessor::sxn_environ);
    try {
        j_setPHPNative(thread1); //TODO move this method call to avoid repeated calls
        nativePHPFn fn1 = userFunctionPHPNativeCall;
        saxonProcessor->registerUserFunctionWithProcessor((void *)saxonProcessor, namespacei, (void *)fn1, (const char *)signature, useNativeTypes);
    } catch (SaxonApiException &e) {
        zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }

}



/*     ============== PHP Interface of   XdmValue =============== */
void xdmValue_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

zend_object *xdmValue_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  xdmValue_object *obj = (xdmValue_object *)ecalloc(
      1, sizeof(xdmValue_object) + zend_object_properties_size(type));
  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xdmValue_object_handlers;

  return &obj->std;
}

void XdmValue_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

PHP_METHOD(XdmValue, __construct) {
  XdmValue *xdmValue = nullptr;
  bool bVal;
  char *sVal;
  int len;
  long iVal;
  double dVal;
  zval *zvalue;

  SaxonProcessor *proc = nullptr;
  // xdmValue_object *obj = (xdmValue_object *) Z_OBJ_P(getThis() TSRMLS_CC);
  /*if (zend_parse_parameters(ZEND_NUM_ARGS() , "z",&zvalue) == SUCCESS) {
      switch (Z_TYPE_P(zvalue)) {
          case IS_FALSE:
          case IS_TRUE:
              bVal = Z_BVAL_P(zvalue);
              xdmValue = new XdmValue(bVal);
              obj = (xdmValue_object *)Z_OBJ_P(getThis() TSRMLS_CC);
              obj->xdmValue = xdmValue;
          break;
          case IS_LONG:
              iVal = Z_LVAL_P(zvalue);
              xdmValue = new XdmValue((int)iVal);
              obj = (xdmValue_object *)Z_OBJ_P(getThis() TSRMLS_CC);
              obj->xdmValue = xdmValue;
          break;
          case IS_STRING:
              sVal = Z_STRVAL_P(zvalue);
              len = Z_STRLEN_P(zvalue);
              xdmValue = new XdmValue("string", sVal);
              obj = (xdmValue_object *)Z_OBJ_P(getThis() TSRMLS_CC);
              obj->xdmValue = xdmValue;
          break;
          case IS_NULL:
              xdmValue = new XdmValue();
              obj = (xdmValue_object *)Z_OBJ_P(getThis() TSRMLS_CC);
              obj->xdmValue = xdmValue;
          break;
          case IS_DOUBLE:
              // TODO: implement this
              //index = (long)Z_DVAL_P(zvalue);
          //break;
          case IS_ARRAY:
          //break;
          case IS_OBJECT:
          //break;
          default:
              obj = nullptr;
              zend_throw_exception(zend_exception_get_default(TSRMLS_C),
  "unknown type specified in XdmValue", 0 TSRMLS_CC);
      }
  }*/
}

PHP_METHOD(XdmValue, __destruct) {

  if (getenv("SAXONC_DEBUG_FLAG")) {
   std::cerr<<"php_xdmValue  destruct"<<std::endl;
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  XdmValue *xdmValue = obj->xdmValue;
  if (xdmValue != nullptr) {
    xdmValue->decrementRefCount();
    if (xdmValue->getRefCount() < 1) {
      xdmValue->resetRelinquishedItems();
      delete xdmValue;
      xdmValue = nullptr;
    } /*else {
       xdmValue->decrementRefCount();
   }    */
  }
}

PHP_METHOD(XdmValue, __toString) {
  XdmValue *xdmValue;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  xdmValue = obj->xdmValue;
  if (xdmValue != nullptr) {
    const char *value = xdmValue->toString();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmValue, getHead) {
  XdmValue *xdmValue;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  xdmValue = obj->xdmValue;
  if (xdmValue != nullptr) {
    xdmValue->incrementRefCountForRelinquishedValue(0); // incremented as the item is still apart of the XdmValue
    XdmItem *item = xdmValue->getHead();
    if (item != nullptr) {
      if (object_init_ex(return_value, xdmItem_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        item->incrementRefCount();
        zend_object *oobj = Z_OBJ_P(return_value);
        xdmItem_object *vobj =
            (xdmItem_object *)((char *)oobj - XtOffsetOf(xdmItem_object, std));
        assert(vobj != nullptr);
        vobj->xdmItem = item;
      }
    } else {
      RETURN_NULL();
    }

  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmValue, itemAt) {
  XdmValue *xdmValue;

  long index;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &index) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  xdmValue = obj->xdmValue;
  if (xdmValue != nullptr) {
    xdmValue->incrementRefCountForRelinquishedValue((unsigned int)index); // incremented as the item is still apart of the XdmValue
    XdmItem *item = xdmValue->itemAt((unsigned int)index);

    if (item != nullptr) {

      if (object_init_ex(return_value, xdmItem_ce) != SUCCESS) {
        RETURN_NULL();
        return;
      } else {
        item->incrementRefCount();
        zend_object *oobj = Z_OBJ_P(return_value);
        xdmItem_object *vobj =
            (xdmItem_object *)((char *)oobj - XtOffsetOf(xdmItem_object, std));

        assert(vobj != nullptr);
        vobj->xdmItem = item;
        return;
      }
    }

  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmValue, size) {
  XdmValue *xdmValue;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  xdmValue = obj->xdmValue;
  int sizei = 0;
  if (xdmValue != nullptr) {
    sizei = xdmValue->size();
  }
  RETURN_LONG(sizei);
}

PHP_METHOD(XdmValue, addXdmItem) {
  XdmValue *xdmValue = nullptr;
  zval *oth = nullptr;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &oth) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmValue_object *obj =
      (xdmValue_object *)((char *)oobj - XtOffsetOf(xdmValue_object, std));
  xdmValue = obj->xdmValue;
  if (xdmValue != nullptr) {
    const char *objName = ZSTR_VAL(Z_OBJCE_P(oth)->name);
    // std::cout<<"test type:"<<(Z_OBJCE_P(oth)->name)<<std::endl;

    if (strcmp(objName, "Saxon\\XdmNode") == 0) {
      zend_object *nobj = Z_OBJ_P(oth);
      xdmNode_object *ooth =
          (xdmNode_object *)((char *)nobj - XtOffsetOf(xdmNode_object, std));
      if (ooth != nullptr) {
        XdmNode *value = ooth->xdmNode;
        if (value != nullptr) {
          xdmValue->addXdmItem((XdmItem *)value);
          value->incrementRefCount();
          return;
        }
      }
    } else if (strcmp(objName, "Saxon\\XdmItem") == 0) {
      zend_object *iobj = Z_OBJ_P(oth);
      xdmItem_object *ooth =
          (xdmItem_object *)((char *)iobj - XtOffsetOf(xdmItem_object, std));
      if (ooth != nullptr) {
        XdmItem *value = ooth->xdmItem;
        if (value != nullptr) {
          xdmValue->addXdmItem(value);
          value->incrementRefCount();
          return;
        }
      }

    } else if (strcmp(objName, "Saxon\\XdmAtomicValue") == 0) {
      zend_object *aobj = Z_OBJ_P(oth);
      xdmAtomicValue_object *ooth =
          (xdmAtomicValue_object *)((char *)aobj -
                                    XtOffsetOf(xdmAtomicValue_object, std));
      if (ooth != nullptr) {
        XdmAtomicValue *value = ooth->xdmAtomicValue;
        if (value != nullptr) {
          xdmValue->addXdmItem((XdmItem *)value);
          value->incrementRefCount();
          return;
        }
      }

    } else {
      // TODO exception
    }
  }
}

/*     ============== PHP Interface of   XdmItem =============== */

void xdmItem_free_storage(zend_object *object) { zend_object_std_dtor(object); }

zend_object *xdmItem_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  xdmItem_object *obj = (xdmItem_object *)ecalloc(
      1, sizeof(xdmItem_object) + zend_object_properties_size(type));

  object_properties_init(&obj->std, type);
  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */

  obj->std.handlers = &xdmItem_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmItem, __construct) {
  XdmItem *xdmItem = nullptr;
  bool bVal;
  char *sVal;
  int len;
  long iVal;
  double dVal;
  zval *zvalue;

  SaxonProcessor *proc = nullptr;

  zval *object = getThis();

  zend_object *zobj = Z_OBJ_P(object);

  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  // saxonProc =  obj->saxonProcessor;
}

void XdmItem_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

PHP_METHOD(XdmItem, __destruct) {
  if (getenv("SAXONC_DEBUG_FLAG")) {
   std::cerr<<"php_XdmItem  destruct"<<std::endl;
  }
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));

  XdmItem *xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    xdmItem->decrementRefCount();
    if (xdmItem->getRefCount() < 1) {
      delete xdmItem;
      xdmItem = nullptr;
    } /*else {
       xdmItem->decrementRefCount();
   }   */
  }
}

PHP_METHOD(XdmItem, getStringValue) {
  XdmItem *xdmItem;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
	const char *value = nullptr;
    try {
    	value = xdmItem->getStringValue();
    } catch (SaxonApiException &e) {
   		zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    }
  }
  RETURN_NULL();
}

/* {{{ __toString()
   Returns the string content */
PHP_METHOD(XdmItem, __toString) {
  XdmItem *xdmItem;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)oobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;
  if (xdmItem != nullptr) {
    const char *value = xdmItem->toString();
    if (value != nullptr) {
      RETURN_STRING(value);
    } else {
      RETURN_NULL();
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmItem, isAtomic) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    bool isAtomic = xdmItem->isAtomic();
    RETURN_BOOL(isAtomic);
  }
  RETURN_BOOL(false);
}

PHP_METHOD(XdmItem, isNode) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr && xdmItem->getType() == XDM_NODE) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

PHP_METHOD(XdmItem, isMap) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr && xdmItem->getType() == XDM_MAP) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}


PHP_METHOD(XdmItem, isArray) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr && xdmItem->getType() == XDM_ARRAY) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}


PHP_METHOD(XdmItem, isFunctionItem) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr && xdmItem->getType() == XDM_FUNCTION_ITEM) {
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

PHP_METHOD(XdmItem, getAtomicValue) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    if (!xdmItem->isAtomic()) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmAtomicValue_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      xdmItem->incrementRefCount();
      // Z_ADDREF_P(return_value);
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmAtomicValue_object *vobj =
          (xdmAtomicValue_object *)((char *)zobj -
                                    XtOffsetOf(xdmAtomicValue_object, std));
      // struct xdmAtomicValue_object* vobj = (struct xdmAtomicValue_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      assert(vobj != nullptr);
      vobj->xdmAtomicValue = (XdmAtomicValue *)xdmItem;
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmItem, getNodeValue) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    if (!xdmItem->isNode()) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      // struct xdmNode_object* vobj = (struct xdmNode_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      xdmItem->incrementRefCount();
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmNode_object *vobj =
          (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
      assert(vobj != nullptr);
      vobj->xdmNode = (XdmNode *)xdmItem;

      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmItem, getMap) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    if (!xdmItem->isMap()) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmMap_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      xdmItem->incrementRefCount();
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmMap_object *vobj =
          (xdmMap_object *)((char *)zobj - XtOffsetOf(xdmMap_object, std));
      assert(vobj != nullptr);
      vobj->xdmMap = (XdmMap *)xdmItem;

      return;
    }
  }
  RETURN_NULL();
}


PHP_METHOD(XdmItem, getArray) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    if (!xdmItem->isArray()) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      xdmItem->incrementRefCount();
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmArray_object *vobj =
          (xdmArray_object *)((char *)zobj - XtOffsetOf(xdmArray_object, std));
      assert(vobj != nullptr);
      vobj->xdmArray = (XdmArray *)xdmItem;

      return;
    }
  }
  RETURN_NULL();
}



PHP_METHOD(XdmItem, getFunctionItem) {
  XdmItem *xdmItem;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmItem_object *obj =
      (xdmItem_object *)((char *)zobj - XtOffsetOf(xdmItem_object, std));
  xdmItem = obj->xdmItem;

  if (xdmItem != nullptr) {
    if (!xdmItem->isFunction()) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      xdmItem->incrementRefCount();
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmFunctionItem_object *vobj =
          (xdmFunctionItem_object *)((char *)zobj - XtOffsetOf(xdmFunctionItem_object, std));
      assert(vobj != nullptr);
      vobj->xdmFunctionItem = (XdmFunctionItem *)xdmItem;

      return;
    }
  }
  RETURN_NULL();
}

/*     ============== PHP Interface of   XdmNode =============== */

void xdmNode_free_storage(zend_object *object) { zend_object_std_dtor(object); }

zend_object *xdmNode_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  xdmNode_object *obj = (xdmNode_object *)ecalloc(
      1, sizeof(xdmNode_object) + zend_object_properties_size(type));
  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);
  obj->std.handlers = &xdmNode_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmNode, __construct) {
  // xdmNode_object *obj = (xdmNode_object *) Z_OBJ_P(getThis() TSRMLS_CC);
}

void XdmNode_destroy_storage(zend_object *object) {

  zend_objects_destroy_object(object);
}

PHP_METHOD(XdmNode, __destruct) {
  // std::cerr<<"php_node  destruct"<<std::endl;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));

  if (obj != nullptr) {
    XdmNode *xdmNode = obj->xdmNode;
    if (xdmNode != nullptr) {
      xdmNode->decrementRefCount();
      if (xdmNode->getRefCount() < 1) {
          delete xdmNode;
          xdmNode = nullptr;
      }
    }
  }
}

PHP_METHOD(XdmNode, getLineNumber){
    XdmNode *xdmNode;
    zend_object *zobj = Z_OBJ_P(getThis());
    xdmNode_object *obj =
        (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
    xdmNode = obj->xdmNode;

    int linenumber = -1;
    if (xdmNode != nullptr) {
        linenumber = xdmNode->getLineNumber();
    }
    RETURN_LONG(linenumber);
}

PHP_METHOD(XdmNode, equals) {
  XdmNode * xdmNode;
  zval *oth = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &oth) == FAILURE) {
    RETURN_FALSE;
  }
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  const char *objName = ZSTR_VAL(Z_OBJCE_P(oth)->name);
  if (xdmNode != nullptr) {
    if (strcmp(objName, "Saxon\\XdmNode") == 0) {
      zend_object *vvobj = Z_OBJ_P(oth);
      xdmNode_object *ooth =
        (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));
      if (ooth != nullptr) {
        XdmNode * other = ooth->xdmNode;
        if (other != nullptr) {
          xdmNode->equals(other);
          RETURN_TRUE;
        }
      }
    }
  }
  RETURN_FALSE;
}


PHP_METHOD(XdmNode, getColumnNumber){
    XdmNode *xdmNode;
    zend_object *zobj = Z_OBJ_P(getThis());
    xdmNode_object *obj =
        (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
    xdmNode = obj->xdmNode;

    int colnumber = -1;
    if (xdmNode != nullptr) {
        colnumber = xdmNode->getColumnNumber();
    }
    RETURN_LONG(colnumber);
}

PHP_METHOD(XdmNode, getStringValue) {
  XdmNode *xdmNode;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    const char *value = xdmNode->getStringValue();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    }
  }
  RETURN_NULL();
}

/* {{{ __toString()
   Returns the string content */
PHP_METHOD(XdmNode, __toString) {
  XdmNode *xdmNode;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)oobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;
  if (xdmNode != nullptr) {
    const char *value = xdmNode->toString();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmNode, getTypedValue) {
  XdmNode *xdmNode;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    XdmValue *typedValue = xdmNode->getTypedValue();
    if (typedValue == nullptr) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      typedValue->incrementRefCount();

      zend_object *zobj = Z_OBJ_P(return_value);
      xdmValue_object *vobj =
          (xdmValue_object *)((char *)zobj - XtOffsetOf(xdmValue_object, std));
      assert(vobj != nullptr);
      vobj->xdmValue = typedValue;
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getNodeName) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    const char *valueStr = xdmNode->getNodeName();
    if (valueStr != nullptr) {
      // char *str = estrdup(valueStr);
      RETURN_STRING(valueStr);
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getLocalName) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    const char *valueStr = xdmNode->getLocalName();
    if (valueStr != nullptr) {
      // char *str = estrdup(valueStr);
      RETURN_STRING(valueStr);
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getBaseURI) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    const char *valueStr = xdmNode->getBaseUri();
    if (valueStr != nullptr) {
      RETURN_STRING(valueStr);
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getNodeKind) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  int nodeKind = 0;
  if (xdmNode != nullptr) {
    nodeKind = xdmNode->getNodeKind();
  }
  RETURN_LONG(nodeKind);
}

PHP_METHOD(XdmNode, isAtomic) { RETURN_FALSE; }

PHP_METHOD(XdmNode, getChildCount) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  int nodeChildCount = 0;
  if (xdmNode != nullptr) {
    nodeChildCount = xdmNode->getChildCount();
  }
  RETURN_LONG(nodeChildCount);
}

PHP_METHOD(XdmNode, getAttributeCount) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  int nodeAttrCount = 0;
  if (xdmNode != nullptr) {
    nodeAttrCount = xdmNode->getAttributeCount();
  }
  RETURN_LONG(nodeAttrCount);
}

PHP_METHOD(XdmNode, getChildNode) {
  long indexi;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &indexi) == FAILURE) {
    RETURN_NULL();
  }

  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    int count = xdmNode->getChildCount();
    if (count == 0) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {

      if (indexi >= 0 && indexi < count) {

        XdmNode *childNode = xdmNode->getChild(indexi, false);

        if (childNode != nullptr) {

          childNode->incrementRefCount();
          zend_object *zobj = Z_OBJ_P(return_value);
          xdmNode_object *vobj =
              (xdmNode_object *)((char *)zobj -
                                 XtOffsetOf(xdmNode_object, std));
          assert(vobj != nullptr);
          vobj->xdmNode = childNode;
          return;
        }
      }
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getChildren) {
  zval retval;
  /*long indexi;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &indexi) == FAILURE) {
    RETURN_NULL();
  } */

  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    int count = xdmNode->getChildCount();
    if (count > 0) {

      XdmNode **childNodes = xdmNode->getChildren(true);

      if (childNodes != nullptr) {
        ///// creating PHP array
        zend_array *ret = zend_new_array(count);
        zend_ulong idx = 0;

        for (int i = 0; i < count; i++) {
          XdmNode *cNode = childNodes[i];
          cNode->incrementRefCount();
          if (object_init_ex(&retval, xdmNode_ce) != SUCCESS) {
            RETURN_NULL();
          }
          zend_object *zobj = Z_OBJ_P(&retval);
          xdmNode_object *vobj =
              (xdmNode_object *)((char *)zobj -
                                 XtOffsetOf(xdmNode_object, std));
          assert(vobj != nullptr);
          vobj->xdmNode = cNode;
          zend_hash_index_add(ret, idx, &retval);
          idx++;
        }
        delete[] childNodes;
        RETVAL_ARR(ret);
        return;
      }
      /// end of creating PHP array
    }
  }

  RETURN_NULL();
}

PHP_METHOD(XdmNode, axisNodes) {
  long axisValue;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &axisValue) == FAILURE) {
    RETURN_NULL();
  }

  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {

      if (axisValue > 0) {

        XdmNode **nodes =
            xdmNode->axisNodes(xdmNode->convertEnumXdmAxis(axisValue));
        int axisCount = xdmNode->axisNodeCount();

        if (axisCount > 0 && nodes != nullptr) {
          XdmValue *resultValue = new XdmValue();
          for (int i = 0; i < axisCount; i++) {
            nodes[i]->incrementRefCount();
            resultValue->addXdmItem(nodes[i]);
          }
          zend_object *zobj = Z_OBJ_P(return_value);
          xdmValue_object *vobj =
              (xdmValue_object *)((char *)zobj -
                                  XtOffsetOf(xdmValue_object, std));
          assert(vobj != nullptr);
          vobj->xdmValue = resultValue;
          return;
        }
      }
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getParent) {
  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    XdmNode *parent = xdmNode->getParent();
    if (parent == nullptr) {
      RETURN_NULL();
      return;
    }
    if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
      RETURN_NULL();
      return;
    } else {
      parent->incrementRefCount();
      // struct xdmNode_object* vobj = (struct xdmNode_object
      // *)Z_OBJ_P(return_value TSRMLS_CC);
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmNode_object *vobj =
          (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
      assert(vobj != nullptr);
      vobj->xdmNode = parent;
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getAttributeNode) {
  zval *retval;
  long indexi = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &indexi) == FAILURE) {
    RETURN_NULL();
  }

  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    int count = xdmNode->getAttributeCount();
    if (count > 0 && indexi < count) {

      XdmNode **attNodes = xdmNode->getAttributeNodes();
      XdmNode *attNode = attNodes[indexi];

      attNode->incrementRefCount();
      if (object_init_ex(return_value, xdmNode_ce) != SUCCESS) {
        RETURN_NULL();
      }
      zend_object *zobj = Z_OBJ_P(return_value);
      xdmNode_object *vobj =
          (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
      assert(vobj != nullptr);
      vobj->xdmNode = attNode;
      for (int i = 0; i < count; i++) {
        if (indexi != i) {
          delete attNodes[i];
        }
      }
      delete[] attNodes;
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getAttributeNodes) {
  zval retval;
  /*if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &indexi) == FAILURE) {
    RETURN_NULL();
  }*/

  XdmNode *xdmNode;
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;

  if (xdmNode != nullptr) {
    int count = xdmNode->getAttributeCount();
    if (count > 0) {
      ///// creating PHP array
      XdmNode **attNodes = xdmNode->getAttributeNodes();
      if (attNodes != nullptr) {
        zend_array *ret = zend_new_array(count);
        zend_ulong idx = 0;

        for (int i = 0; i < count; i++) {
          XdmNode *attNode = attNodes[i];
          attNode->incrementRefCount();
          if (object_init_ex(&retval, xdmNode_ce) != SUCCESS) {
            RETURN_NULL();
          }
          zend_object *zobj = Z_OBJ_P(&retval);
          xdmNode_object *vobj =
              (xdmNode_object *)((char *)zobj -
                                 XtOffsetOf(xdmNode_object, std));
          assert(vobj != nullptr);
          vobj->xdmNode = attNode;
          zend_hash_index_add(ret, idx, &retval);
          idx++;
        }
        delete[] attNodes;
        RETVAL_ARR(ret);
        return;
      }
      /// end of creating PHP array
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmNode, getAttributeValue) {
  char *name;
  long len1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &name, &len1) == FAILURE) {
    RETURN_NULL();
  }
  XdmNode *xdmNode; // c++ object
  zend_object *zobj = Z_OBJ_P(getThis());
  xdmNode_object *obj =
      (xdmNode_object *)((char *)zobj - XtOffsetOf(xdmNode_object, std));
  xdmNode = obj->xdmNode;
  if (xdmNode != nullptr && name != nullptr) {

    const char *value = xdmNode->getAttributeValue(name);
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    }
  }
  RETURN_NULL();
}

/*     ============== PHP Interface of   XdmAtomicValue =============== */

void xdmAtomicValue_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

void XdmAtomicValue_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xdmAtomicValue_create_handler(zend_class_entry *type) {

  xdmAtomicValue_object *obj = (xdmAtomicValue_object *)ecalloc(
      1, sizeof(xdmAtomicValue_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xdmAtomicValue_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmAtomicValue, __construct) {
  /* XdmAtomicValue *xdmValue = nullptr;
   bool bVal;
   char * sVal;
   int len;
   long iVal;
   double dVal;
   zval *zvalue;*/

  // xdmAtomicValue_object *obj = (xdmAtomicValue_object *) Z_OBJ_P(getThis()
  // TSRMLS_CC);
}

PHP_METHOD(XdmAtomicValue, __destruct) {
  if (getenv("SAXONC_DEBUG_FLAG")) {
   std::cerr<<"php_xdmAtomicValue  destruct"<<std::endl;
  }
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));

  XdmAtomicValue *xdmValue = obj->xdmAtomicValue;
  if (xdmValue != nullptr) {
    xdmValue->decrementRefCount();
    if (xdmValue->getRefCount() < 1) {
      delete xdmValue;
      xdmValue = nullptr;
    } else {
      // xdmValue->decrementRefCount();
    }
  }
}

PHP_METHOD(XdmAtomicValue, getBooleanValue) {
  XdmAtomicValue *xdmAtomicValue;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;

  bool resultb = false;
  if (xdmAtomicValue != nullptr) {
    resultb = xdmAtomicValue->getBooleanValue();
  }
  RETURN_BOOL(resultb);
}

PHP_METHOD(XdmAtomicValue, getDoubleValue) {
  XdmAtomicValue *xdmAtomicValue;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;

  double resultb = 0;
  if (xdmAtomicValue != nullptr) {
    resultb = xdmAtomicValue->getDoubleValue();
  }
  RETURN_DOUBLE(resultb);
}

PHP_METHOD(XdmAtomicValue, getLongValue) {
  XdmAtomicValue *xdmAtomicValue;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;

  long result = 0;
  if (xdmAtomicValue != nullptr) {
    result = xdmAtomicValue->getLongValue();
  }
  RETURN_LONG(result);
}

PHP_METHOD(XdmAtomicValue, getStringValue) {
  XdmAtomicValue *xdmAtomicValue;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;

  SaxonProcessor *saxonProc;
  zend_object *zzobj = Z_OBJ_P(getThis());

  saxonProcessor_object *obj2 =
      (saxonProcessor_object *)((char *)zzobj -
                                XtOffsetOf(xdmAtomicValue_object, std));

  saxonProc = obj2->saxonProcessor;

  if (xdmAtomicValue != nullptr) {
    const char *value = xdmAtomicValue->getStringValue();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    }
  }
  RETURN_NULL();
}

PHP_METHOD(XdmAtomicValue, getPrimitiveTypeName) {
  XdmAtomicValue *xdmAtomicValue;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)zobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;

  if (xdmAtomicValue != nullptr) {
    const char *valueStr = xdmAtomicValue->getPrimitiveTypeName();
    if (valueStr != nullptr) {
      // char *str = estrdup(valueStr);
      RETURN_STRING(valueStr);
    }
  }
  RETURN_NULL();
}

/* {{{ __toString()
   Returns the string content */
PHP_METHOD(XdmAtomicValue, __toString) {
  XdmAtomicValue *xdmAtomicValue;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmAtomicValue_object *obj =
      (xdmAtomicValue_object *)((char *)oobj -
                                XtOffsetOf(xdmAtomicValue_object, std));
  xdmAtomicValue = obj->xdmAtomicValue;
  if (xdmAtomicValue != nullptr) {
    const char *value = xdmAtomicValue->getStringValue();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmAtomicValue, isAtomic) { RETURN_TRUE; }

/*     ============== PHP Interface of   XdmFunctionItem =============== */

void xdmFunctionItem_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

void XdmFunctionItem_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xdmFunctionItem_create_handler(zend_class_entry *type) {

  xdmFunctionItem_object *obj = (xdmFunctionItem_object *)ecalloc(
      1, sizeof(xdmFunctionItem_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xdmFunctionItem_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmFunctionItem, __construct) {
  /* XdmAtomicValue *xdmValue = nullptr;
   bool bVal;
   char * sVal;
   int len;
   long iVal;
   double dVal;
   zval *zvalue;*/

  // xdmAtomicValue_object *obj = (xdmAtomicValue_object *) Z_OBJ_P(getThis()
  // TSRMLS_CC);
}

PHP_METHOD(XdmFunctionItem, __destruct) {
  if (getenv("SAXONC_DEBUG_FLAG")) {
   std::cerr<<"php_xdmFunctionItem  destruct"<<std::endl;
  }
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)zobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));

  XdmFunctionItem *xdmValue = obj->xdmFunctionItem;
  if (xdmValue != nullptr) {
    xdmValue->decrementRefCount();
    if (xdmValue->getRefCount() < 1) {
      delete xdmValue;
      xdmValue = nullptr;
    } else {
      // xdmValue->decrementRefCount();
    }
  }
}

PHP_METHOD(XdmFunctionItem, __toString) {
  XdmFunctionItem *xdmFunctionItem;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)oobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));
  xdmFunctionItem = obj->xdmFunctionItem;
  if (xdmFunctionItem != nullptr) {
    const char *value = xdmFunctionItem->toString();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmFunctionItem, getName) {
  XdmFunctionItem *xdmFunctionItem;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)oobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));
  xdmFunctionItem = obj->xdmFunctionItem;
  if (xdmFunctionItem != nullptr) {
    const char *value = xdmFunctionItem->getName();
    if (value != nullptr) {
      RETURN_STRING(value);
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmFunctionItem, getArity) {
  XdmFunctionItem *xdmFunctionItem;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)oobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));
  xdmFunctionItem = obj->xdmFunctionItem;
  if (xdmFunctionItem != nullptr) {
    int value = xdmFunctionItem->getArity();
    RETURN_LONG(value);
  } else {
    RETURN_LONG(-1);
  }
}

PHP_METHOD(XdmFunctionItem, getSystemFunction) {

  XdmFunctionItem *xdmFunctionItem;
  char *functionName = nullptr;
  zval *proc_zval;
  SaxonProcessor *proc = nullptr;
  size_t len1;

  long arity;

  if (ZEND_NUM_ARGS() != 3) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zsl", &proc_zval, &functionName,
                            &len1, &arity) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)oobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));
  xdmFunctionItem = obj->xdmFunctionItem;

  if (xdmFunctionItem != nullptr) {
    if (Z_TYPE_P(proc_zval) == IS_NULL) {
    }
    zend_object *ooth = Z_OBJ_P(proc_zval);
    saxonProcessor_object *nobj =
        (saxonProcessor_object *)((char *)ooth -
                                  XtOffsetOf(saxonProcessor_object, std));
    if (nobj != nullptr) {
      proc = (SaxonProcessor *)nobj->saxonProcessor;
      XdmFunctionItem *result =
          xdmFunctionItem->getSystemFunction(proc, functionName, arity);

      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmFunctionItem_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *ooth = Z_OBJ_P(return_value);

          xdmFunctionItem_object *vobj =
              (xdmFunctionItem_object *)((char *)ooth -
                                         XtOffsetOf(xdmFunctionItem_object,
                                                    std));
          assert(vobj != nullptr);
          vobj->xdmFunctionItem = result;
          return;
        }
      }
    }
  }

  RETURN_NULL();
}

PHP_METHOD(XdmFunctionItem, call) {
  XdmFunctionItem *xdmFunctionItem;
  char *functionName = nullptr;
  zval *proc_zval;
  SaxonProcessor *proc = nullptr;
  HashTable *arr_hash;

  zval *arguments_zval;
  XdmValue **arguments = nullptr;
  int argument_length = 0;
  zend_string *key;
  zval *val;
  long num_key;

  if (ZEND_NUM_ARGS() != 3) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "za", &proc_zval,
                            &arguments_zval) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmFunctionItem_object *obj =
      (xdmFunctionItem_object *)((char *)oobj -
                                 XtOffsetOf(xdmFunctionItem_object, std));
  xdmFunctionItem = obj->xdmFunctionItem;

  if (xdmFunctionItem != nullptr) {

    zend_object *ooth = Z_OBJ_P(proc_zval);
    saxonProcessor_object *nobj =
        (saxonProcessor_object *)((char *)ooth -
                                  XtOffsetOf(saxonProcessor_object, std));
    if (nobj != nullptr) {
      proc = (SaxonProcessor *)nobj->saxonProcessor;

      arr_hash = Z_ARRVAL_P(arguments_zval);
      argument_length = zend_hash_num_elements(arr_hash);

      arguments = new XdmValue *[argument_length];
      ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {
        if (Z_TYPE_P(val) != IS_NULL) {
          const char *objName = ZSTR_VAL(Z_OBJCE_P(val)->name);

          if (strcmp(objName, "Saxon\\XdmNode") == 0) {
            zend_object *ooth = Z_OBJ_P(val);
            xdmNode_object *nobj =
                (xdmNode_object *)((char *)ooth -
                                   XtOffsetOf(xdmNode_object, std));
            if (nobj != nullptr) {
              XdmValue *value = (XdmValue *)nobj->xdmNode;
              arguments[num_key] = value;
            }
          } else if (strcmp(objName, "Saxon\\XdmAtomicValue") == 0) {
            zend_object *ooth = Z_OBJ_P(val);
            xdmAtomicValue_object *nobj =
                (xdmAtomicValue_object *)((char *)ooth -
                                          XtOffsetOf(xdmAtomicValue_object,
                                                     std));
            if (nobj != nullptr) {
              XdmValue *value = (XdmValue *)nobj->xdmAtomicValue;
              arguments[num_key] = value;
            }
          } else if (strcmp(objName, "Saxon\\XdmValue") == 0) {
            zend_object *ooth = Z_OBJ_P(val);
            xdmValue_object *nobj =
                (xdmValue_object *)((char *)ooth -
                                    XtOffsetOf(xdmValue_object, std));
            if (nobj != nullptr) {
              XdmValue *value = nobj->xdmValue;
              arguments[num_key] = value;
            }
          } else { // TODO error warning}
          }
        } else {
          // TODO error warning
        }
      }
      ZEND_HASH_FOREACH_END();

      XdmValue *result =
          xdmFunctionItem->call(proc, arguments, argument_length);

      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *ooth = Z_OBJ_P(return_value);

          xdmValue_object *vobj =
              (xdmValue_object *)((char *)ooth -
                                  XtOffsetOf(xdmValue_object, std));
          assert(vobj != nullptr);
          vobj->xdmValue = result;
          return;
        }
      }
    }
  }

  RETURN_NULL();
}

/*     ============== PHP Interface of   XdmMap =============== */

void xdmMap_free_storage(zend_object *object) { zend_object_std_dtor(object); }

void XdmMap_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xdmMap_create_handler(zend_class_entry *type) {

  xdmMap_object *obj = (xdmMap_object *)ecalloc(
      1, sizeof(xdmMap_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xdmMap_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmMap, __construct) {
  /* XdmAtomicValue *xdmValue = nullptr;
   bool bVal;
   char * sVal;
   int len;
   long iVal;
   double dVal;
   zval *zvalue;*/

  // xdmAtomicValue_object *obj = (xdmAtomicValue_object *) Z_OBJ_P(getThis()
  // TSRMLS_CC);
}

PHP_METHOD(XdmMap, __destruct) {
  // std::cerr<<"php_atomicValue  destruct"<<std::endl;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmMap_object *obj =
      (xdmMap_object *)((char *)zobj - XtOffsetOf(xdmMap_object, std));

  XdmMap *xdmmap = obj->xdmMap;
  if (xdmmap != nullptr) {
    xdmmap->decrementRefCount();
    if (xdmmap->getRefCount() < 1) {
      delete xdmmap;
      xdmmap = nullptr;
    } else {
      // xdmValue->decrementRefCount();
    }
  }
}

PHP_METHOD(XdmMap, __toString) {
  XdmMap *xdmMap;
  // zend_string *str;

  if (zend_parse_parameters_none() == FAILURE) {
    RETURN_THROWS();
  }
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {
    const char *value = xdmMap->toString();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_EMPTY_STRING();
    }
  } else {
    RETURN_EMPTY_STRING();
  }
}

PHP_METHOD(XdmMap, mapSize) {
  XdmMap *xdmMap;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {
    int value = xdmMap->mapSize();
    RETURN_LONG(value);
  } else {
    RETURN_LONG(-1);
  }
}

PHP_METHOD(XdmMap, get) {
  XdmMap *xdmMap = nullptr;
  XdmValue *result = nullptr;
  zval *key_zval = nullptr;
  long lkey = -1;
  char *skey = nullptr;
  size_t len1;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &skey, &len1) == FAILURE) {
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key_zval) == FAILURE) {

      if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &lkey) == FAILURE) {

        RETURN_NULL();
      }
    }
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {
    if (key_zval != nullptr) {

      const char *objName = ZSTR_VAL(Z_OBJCE_P(key_zval)->name);

      if (strcmp(objName, "Saxon\\XdmAtomicValue") == 0) {
        zend_object *nobj = Z_OBJ_P(key_zval);

        xdmAtomicValue_object *ooth =
            (xdmAtomicValue_object *)((char *)nobj -
                                      XtOffsetOf(xdmAtomicValue_object, std));
        if (ooth != nullptr) {

          XdmAtomicValue *value = ooth->xdmAtomicValue;

          if (value != nullptr) {

            result = xdmMap->get(value); // call back to C++ API
            if (result != nullptr) {
              result->incrementRefCount();
              if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
                RETURN_NULL();
              } else {
                zend_object *vvobj = Z_OBJ_P(return_value);
                xdmValue_object *vobj =
                    (xdmValue_object *)((char *)vvobj -
                                        XtOffsetOf(xdmValue_object, std));
                assert(vobj != nullptr);
                vobj->xdmValue = result;
                return;
              }
            } else {
              RETURN_NULL();
            }
          }
        }
      } else {
        RETURN_NULL();
      }

    } else if (lkey != -1) {
      result = xdmMap->get(lkey);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmValue_object *vobj =
              (xdmValue_object *)((char *)vvobj -
                                  XtOffsetOf(xdmValue_object, std));
          assert(vobj != nullptr);
          vobj->xdmValue = result;
          return;
        }
      } else {
        RETURN_NULL();
      }

    } else if (skey != nullptr) {
      result = xdmMap->get(skey);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmValue_object *vobj =
              (xdmValue_object *)((char *)vvobj -
                                  XtOffsetOf(xdmValue_object, std));
          assert(vobj != nullptr);
          vobj->xdmValue = result;
          return;
        }
      } else {
        RETURN_NULL();
      }

    } else {

      RETURN_NULL();
    }
  }
}

PHP_METHOD(XdmMap, put) {
  XdmMap *xdmMap;
  zval *key_zval = nullptr;
  zval *value_zval = nullptr;
  long lkey = -1;
  char *skey = nullptr;
  size_t len1;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &key_zval, &value_zval) ==
      FAILURE) {
    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {
    zend_object *kobj = Z_OBJ_P(key_zval);
    XdmAtomicValue *c_key = nullptr;
    XdmValue *c_value = nullptr;
    xdmAtomicValue_object *ooth =
        (xdmAtomicValue_object *)((char *)kobj -
                                  XtOffsetOf(xdmAtomicValue_object, std));
    if (ooth != nullptr) {
      c_key = ooth->xdmAtomicValue;
    }

    zend_object *vobj = Z_OBJ_P(value_zval);

    xdmAtomicValue_object *vooth =
        (xdmAtomicValue_object *)((char *)vobj -
                                  XtOffsetOf(xdmAtomicValue_object, std));
    if (vooth != nullptr) {
      c_value = vooth->xdmAtomicValue;
    }

    if (c_value != nullptr && c_key != nullptr) {
      XdmMap *result = xdmMap->put(c_key, c_value);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmMap_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmMap_object *vobj =
              (xdmMap_object *)((char *)vvobj - XtOffsetOf(xdmMap_object, std));
          assert(vobj != nullptr);
          vobj->xdmMap = result;
          return;
        }
      }
    }

  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmMap, isEmpty) {
  XdmMap *xdmMap;
  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {
    bool result = xdmMap->isEmpty();
    RETURN_BOOL(result);
  }
  RETURN_BOOL(false);
}

PHP_METHOD(XdmMap, containsKey) {
  XdmMap *xdmMap;
  zval *key_zval = nullptr;
  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key_zval) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmMap_object *obj =
      (xdmMap_object *)((char *)oobj - XtOffsetOf(xdmMap_object, std));
  xdmMap = obj->xdmMap;
  if (xdmMap != nullptr) {

    zend_object *kobj = Z_OBJ_P(key_zval);
    XdmAtomicValue *c_key = nullptr;
    xdmAtomicValue_object *ooth =
        (xdmAtomicValue_object *)((char *)kobj -
                                  XtOffsetOf(xdmAtomicValue_object, std));
    if (ooth != nullptr) {
      c_key = ooth->xdmAtomicValue;
      bool result = xdmMap->containsKey(c_key);
      RETURN_BOOL(result);
    }
  }
  RETURN_BOOL(false);
}
/*
TODO implement these methods
*/
PHP_METHOD(XdmMap, keys) {}

/*PHP_METHOD(XdmMap, values)
{


} */

/*     ============== PHP Interface of   XdmArray =============== */

void xdmArray_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

void XdmArray_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xdmArray_create_handler(zend_class_entry *type) {

  xdmArray_object *obj = (xdmArray_object *)ecalloc(
      1, sizeof(xdmArray_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std,
                       type); /* take care of the zend_object also ! */
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xdmArray_object_handlers;

  return &obj->std;
}

PHP_METHOD(XdmArray, __construct) {
  /* XdmAtomicValue *xdmValue = nullptr;
   bool bVal;
   char * sVal;
   int len;
   long iVal;
   double dVal;
   zval *zvalue;*/

  // xdmAtomicValue_object *obj = (xdmAtomicValue_object *) Z_OBJ_P(getThis()
  // TSRMLS_CC);
}

PHP_METHOD(XdmArray, __destruct) {
  // std::cerr<<"php_atomicValue  destruct"<<std::endl;
  zval *object = getThis();
  zend_object *zobj = Z_OBJ_P(object);

  xdmArray_object *obj =
      (xdmArray_object *)((char *)zobj - XtOffsetOf(xdmArray_object, std));

  XdmArray *xdmArray = obj->xdmArray;
  if (xdmArray != nullptr) {
    xdmArray->decrementRefCount();
    if (xdmArray->getRefCount() < 1) {
      delete xdmArray;
      xdmArray = nullptr;
    } else {
      // xdmValue->decrementRefCount();
    }
  }
}

PHP_METHOD(XdmArray, __toString) {
  XdmArray *xdmArray;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (xdmArray != nullptr) {
    const char *value = xdmArray->toString();
    if (value != nullptr) {
      RETVAL_STRING(value);
      SaxonProcessor::deleteString(value);
      return;
    } else {
      RETURN_NULL();
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XdmArray, get) {
  XdmArray *xdmArray = nullptr;
  long lkey = -1;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &lkey) == FAILURE) {

    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (xdmArray != nullptr && lkey != -1) {
    XdmValue *result = xdmArray->get(lkey);
    if (result != nullptr) {
      result->incrementRefCount();
      if (object_init_ex(return_value, xdmValue_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        zend_object *vvobj = Z_OBJ_P(return_value);
        xdmValue_object *vobj =
            (xdmValue_object *)((char *)vvobj -
                                XtOffsetOf(xdmValue_object, std));
        assert(vobj != nullptr);
        vobj->xdmValue = result;
        return;
      }
    } else {
      RETURN_NULL();
    }

  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(XdmArray, put) {
  XdmArray *xdmArray;
  long lkey = -1;
  zval *value_zval = nullptr;

  if (ZEND_NUM_ARGS() != 2) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &lkey, value_zval) ==
      FAILURE) {

    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (lkey != -1 && value_zval != nullptr) {
    zend_object *vobj = Z_OBJ_P(value_zval);

    xdmValue_object *vooth =
        (xdmValue_object *)((char *)vobj - XtOffsetOf(xdmValue_object, std));
    if (vooth != nullptr) {
      XdmValue *c_value = vooth->xdmValue;

      XdmArray *result = xdmArray->put(lkey, c_value);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmArray_object *vobj =
              (xdmArray_object *)((char *)vvobj -
                                  XtOffsetOf(xdmArray_object, std));
          assert(vobj != nullptr);
          vobj->xdmArray = result;
          return;
        }
      } else {
        RETURN_NULL();
      }
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(XdmArray, addMember) {
  XdmArray *xdmArray;
  long lkey = -1;
  zval *value_zval = nullptr;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", value_zval) == FAILURE) {

    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (lkey != -1 && value_zval != nullptr) {
    zend_object *vobj = Z_OBJ_P(value_zval);

    xdmValue_object *vooth =
        (xdmValue_object *)((char *)vobj - XtOffsetOf(xdmValue_object, std));
    if (vooth != nullptr) {
      XdmValue *c_value = vooth->xdmValue;

      XdmArray *result = xdmArray->put(lkey, c_value);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmArray_object *vobj =
              (xdmArray_object *)((char *)vvobj -
                                  XtOffsetOf(xdmArray_object, std));
          assert(vobj != nullptr);
          vobj->xdmArray = result;
          return;
        }
      } else {
        RETURN_NULL();
      }
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(XdmArray, concat) {
  XdmArray *xdmArray;
  long lkey = -1;
  zval *value_zval = nullptr;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", value_zval) == FAILURE) {

    RETURN_NULL();
  }

  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (lkey != -1 && value_zval != nullptr) {
    zend_object *vobj = Z_OBJ_P(value_zval);

    xdmArray_object *vooth =
        (xdmArray_object *)((char *)vobj - XtOffsetOf(xdmArray_object, std));
    if (vooth != nullptr) {
      XdmArray *c_value = vooth->xdmArray;

      XdmArray *result = xdmArray->concat(c_value);
      if (result != nullptr) {
        result->incrementRefCount();
        if (object_init_ex(return_value, xdmArray_ce) != SUCCESS) {
          RETURN_NULL();
        } else {
          zend_object *vvobj = Z_OBJ_P(return_value);
          xdmArray_object *vobj =
              (xdmArray_object *)((char *)vvobj -
                                  XtOffsetOf(xdmArray_object, std));
          assert(vobj != nullptr);
          vobj->xdmArray = result;
          return;
        }
      } else {
        RETURN_NULL();
      }
    }
  } else {

    RETURN_NULL();
  }
}

PHP_METHOD(XdmArray, getArity) {
  XdmArray *xdmArray = nullptr;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (xdmArray != nullptr) {
    int value = xdmArray->getArity();
    RETURN_LONG(value);
  } else {
    RETURN_LONG(-1);
  }
}

PHP_METHOD(XdmArray, arrayLength) {
  XdmArray *xdmArray = nullptr;
  zend_object *oobj = Z_OBJ_P(getThis());
  xdmArray_object *obj =
      (xdmArray_object *)((char *)oobj - XtOffsetOf(xdmArray_object, std));
  xdmArray = obj->xdmArray;
  if (xdmArray != nullptr) {
    int value = xdmArray->arrayLength();
    RETURN_LONG(value);
  } else {
    RETURN_LONG(-1);
  }
}

/*PHP_ME(XdmArray,  values,      nullptr, ZEND_ACC_PUBLIC) */


// =============================================================

zend_function_entry SaxonProcessor_methods[] = {
    PHP_ME(SaxonProcessor, __construct, arginfo_saxonproc_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(SaxonProcessor, __destruct,arginfo_saxonc_destruct,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, createAtomicValue, arginfo_saxonc_createAtomicValue, ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, parseXmlFromString,arginfo_saxonc_parseXmlFromString,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, parseXmlFromFile,arginfo_saxonc_parseXmlFromFile,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, parseXmlFromUri,arginfo_saxonc_parseXmlFromUri,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, parseJsonFromFile,arginfo_saxonc_parseJsonFromFile,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, parseJsonFromString,arginfo_saxonc_parseJsonFromString,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, setcwd, arginfo_saxonc_setcwd,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, getcwd, arginfo_saxonc_getcwd,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, isLicensed, arginfo_saxonc_isLicensed, ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, getSaxonEdition, arginfo_saxonc_getSaxonEdition, ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, newXPathProcessor, arginfo_saxonc_newXPathProcessor, ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, newDocumentBuilder,arginfo_saxonc_newDocumentBuilder,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor,newXslt30Processor,arginfo_saxonc_newXslt30Processor,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, newXQueryProcessor,arginfo_saxonc_newXQueryProcessor,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, newSchemaValidator,arginfo_saxonc_newSchemaValidator,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, newXsdCompiler,arginfo_saxonc_newXsdCompiler,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor,setCatalog,arginfo_saxonc_setCatalog,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, setCatalogFiles,arginfo_saxonc_setCatalogFiles,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor,setConfigurationProperty,arginfo_saxonc_setConfigurationProperty,ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor,  registerUserFunction,   arginfo_saxonc_registerUserFunction, ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, version, arginfo_saxonc_version, ZEND_ACC_PUBLIC)
    PHP_ME(SaxonProcessor, isSchemaAware, arginfo_saxonc_isSchemaAware,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, EQNameToClarkName,arginfo_saxonc_EQNameToClarkName,ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, clarkNameToEQName,arginfo_saxonc_clarkNameToEQName, ZEND_ACC_PUBLIC)
	PHP_ME(SaxonProcessor, release,arginfo_saxonc_release,ZEND_ACC_PUBLIC)
	{nullptr, nullptr, nullptr}};

zend_function_entry xdmValue_methods[] = {
    PHP_ME(XdmValue, __construct, arginfo_saxonc_xdmval_construct,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(XdmValue, __destruct, arginfo_saxonc_xdmval_destruct,ZEND_ACC_PUBLIC)
    PHP_ME(XdmValue, __toString, arginfo_saxonc_xdmval_toString,ZEND_ACC_PUBLIC)
    PHP_ME(XdmValue, getHead, arginfo_saxonc_xdmval_getHead,ZEND_ACC_PUBLIC)
    PHP_ME(XdmValue, itemAt, arginfo_saxonc_xdmval_itemAt,ZEND_ACC_PUBLIC)
    PHP_ME(XdmValue, size, arginfo_saxonc_xdmval_size,ZEND_ACC_PUBLIC)
    PHP_ME(XdmValue, addXdmItem,arginfo_saxonc_xdmval_addXdmItem,ZEND_ACC_PUBLIC)
    {nullptr, nullptr, nullptr}};

zend_function_entry xdmItem_methods[] = {
  PHP_ME(XdmItem, __construct, arginfo_saxonc_xdmitem_construct,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
  PHP_ME(XdmItem, __destruct, arginfo_saxonc_xdmitem_destruct,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, __toString,arginfo_saxonc_xdmitem_toString,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, getStringValue,arginfo_saxonc_xdmitem_getStringValue, ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, isAtomic, arginfo_saxonc_xdmitem_isAtomic,ZEND_ACC_PUBLIC)
	PHP_ME(XdmItem, isNode, arginfo_saxonc_xdmitem_isNode,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, isMap, arginfo_saxonc_xdmitem_isMap,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, isArray, arginfo_saxonc_xdmitem_isArray,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, isFunctionItem, arginfo_saxonc_xdmitem_isFunctionItem,ZEND_ACC_PUBLIC)
  PHP_ME(XdmItem, getAtomicValue,arginfo_saxonc_xdmitem_getAtomicValue,ZEND_ACC_PUBLIC)
	PHP_ME(XdmItem, getNodeValue,arginfo_saxonc_xdmitem_getNodeValue,ZEND_ACC_PUBLIC)
	PHP_ME(XdmItem, getMap,arginfo_saxonc_xdmitem_getMap,ZEND_ACC_PUBLIC)
	PHP_ME(XdmItem, getArray,arginfo_saxonc_xdmitem_getArray,ZEND_ACC_PUBLIC)
	PHP_ME(XdmItem, getFunctionItem,arginfo_saxonc_xdmitem_getFunctionItem,ZEND_ACC_PUBLIC)
    {nullptr, nullptr, nullptr}};

zend_function_entry xdmNode_methods[] = {
    PHP_ME(XdmNode, __construct, arginfo_saxonc_xdmnode_construct,ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
      PHP_ME(XdmNode, __destruct,arginfo_saxonc_xdmnode_destruct,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, __toString, arginfo_saxonc_xdmnode_toString,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getBaseURI,arginfo_saxonc_xdmnode_getBaseURI,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getStringValue,arginfo_saxonc_xdmnode_getStringValue, ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getNodeKind, arginfo_saxonc_xdmnode_getNodeKind,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getNodeName, arginfo_saxonc_xdmnode_getNodeName,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, equals, arginfo_saxonc_xdmnode_equals,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getLocalName,arginfo_saxonc_xdmnode_getLocalName,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getColumnNumber, arginfo_saxonc_xdmnode_getColumnNumber,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getLineNumber,arginfo_saxonc_xdmnode_getLineNumber,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, isAtomic, arginfo_saxonc_xdmnode_isAtomic,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getChildCount,arginfo_saxonc_xdmnode_getChildCount,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, axisNodes,arginfo_saxonc_xdmnode_axisNodes,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getAttributeCount,arginfo_saxonc_xdmnode_getAttributeCount,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getChildNode,arginfo_saxonc_xdmnode_getChildNode,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getChildren,arginfo_saxonc_xdmnode_getChildren,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getParent,arginfo_saxonc_xdmnode_getParent,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getAttributeNode,arginfo_saxonc_xdmnode_getAttributeNode,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getAttributeNodes,arginfo_saxonc_xdmnode_getAttributeNodes,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getAttributeValue,arginfo_saxonc_xdmnode_getAttributeValue,ZEND_ACC_PUBLIC)
      PHP_ME(XdmNode, getTypedValue,arginfo_saxonc_xdmnode_getTypedValue,ZEND_ACC_PUBLIC){
                                                        nullptr, nullptr,
                                                        nullptr}};

zend_function_entry xdmAtomicValue_methods[] = {
    PHP_ME(XdmAtomicValue, __construct, arginfo_saxonc_xdmatomic_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
        PHP_ME(XdmAtomicValue, __destruct, arginfo_saxonc_xdmatomic_destruct,
               ZEND_ACC_PUBLIC) PHP_ME(XdmAtomicValue, __toString,
                                       arginfo_saxonc_xdmatomic_toString,
                                       ZEND_ACC_PUBLIC)
            PHP_ME(XdmAtomicValue, getStringValue,
                   arginfo_saxonc_xdmatomic_getStringValue,
                   ZEND_ACC_PUBLIC) PHP_ME(XdmAtomicValue, isAtomic,
                                           arginfo_saxonc_xdmatomic_isAtomic,
                                           ZEND_ACC_PUBLIC)
                PHP_ME(XdmAtomicValue, getBooleanValue,
                       arginfo_saxonc_xdmatomic_getBooleanValue,
                       ZEND_ACC_PUBLIC)
                    PHP_ME(XdmAtomicValue, getDoubleValue,
                           arginfo_saxonc_xdmatomic_getDoubleValue,
                           ZEND_ACC_PUBLIC)
                        PHP_ME(XdmAtomicValue, getLongValue,
                               arginfo_saxonc_xdmatomic_getLongValue,
                               ZEND_ACC_PUBLIC)
                            PHP_ME(
                                XdmAtomicValue, getPrimitiveTypeName,
                                arginfo_saxonc_xdmatomic_getPrimitiveTypeName,
                                ZEND_ACC_PUBLIC){nullptr, nullptr, nullptr}};

zend_function_entry xdmFunctionItem_methods[] = {
    PHP_ME(XdmFunctionItem, __construct, arginfo_saxonc_xdmfunc_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
        PHP_ME(XdmFunctionItem, __destruct, arginfo_saxonc_xdmfunc_destruct,
               ZEND_ACC_PUBLIC) PHP_ME(XdmFunctionItem, __toString,
                                       arginfo_saxonc_xdmfunc_toString,
                                       ZEND_ACC_PUBLIC)
            PHP_ME(XdmFunctionItem, getName, arginfo_saxonc_xdmfunc_getName,
                   ZEND_ACC_PUBLIC) PHP_ME(XdmFunctionItem, getArity,
                                           arginfo_saxonc_xdmfunc_getArity,
                                           ZEND_ACC_PUBLIC)
                PHP_ME(XdmFunctionItem, getSystemFunction,
                       arginfo_saxonc_xdmfunc_getSystemFunction,
                       ZEND_ACC_PUBLIC)
                    PHP_ME(XdmFunctionItem, call, arginfo_saxonc_xdmfunc_call,
                           ZEND_ACC_PUBLIC){nullptr, nullptr, nullptr}};

zend_function_entry xdmMap_methods[] = {
    PHP_ME(XdmMap, __construct, arginfo_saxonc_xdmmap_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
        PHP_ME(XdmMap, __destruct, arginfo_saxonc_xdmmap_destruct,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, __toString,arginfo_saxonc_xdmmap_toString,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, mapSize, arginfo_saxonc_xdmmap_mapSize,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, get, arginfo_saxonc_xdmmap_get, ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, put, arginfo_saxonc_xdmmap_put,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, isEmpty, arginfo_saxonc_xdmmap_isEmpty,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, containsKey,arginfo_saxonc_xdmmap_containsKey,ZEND_ACC_PUBLIC)
        PHP_ME(XdmMap, keys, arginfo_saxonc_xdmmap_keys,ZEND_ACC_PUBLIC)
    /*PHP_ME(XdmMap,  asMap,      arginfo_saxonc_xdmmap_asMap, ZEND_ACC_PUBLIC)
    PHP_ME(XdmMap,  values,      arginfo_saxonc_xdmmap_values,
    ZEND_ACC_PUBLIC)*/
    {nullptr, nullptr, nullptr}};

zend_function_entry xdmArray_methods[] = {
    PHP_ME(XdmArray, __construct, arginfo_saxonc_xdmarray_construct,
           ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
        PHP_ME(XdmArray, __destruct, arginfo_saxonc_xdmarray_destruct,
               ZEND_ACC_PUBLIC) PHP_ME(XdmArray, __toString,
                                       arginfo_saxonc_xdmarray_toString,
                                       ZEND_ACC_PUBLIC)
            PHP_ME(XdmArray, get, arginfo_saxonc_xdmarray_get, ZEND_ACC_PUBLIC)
                PHP_ME(XdmArray, put, arginfo_saxonc_xdmarray_put,
                       ZEND_ACC_PUBLIC)
                    PHP_ME(XdmArray, addMember,
                           arginfo_saxonc_xdmarray_addMember, ZEND_ACC_PUBLIC)
                        PHP_ME(XdmArray, concat, arginfo_saxonc_xdmarray_concat,
                               ZEND_ACC_PUBLIC)
    // PHP_ME(XdmArray,  values,      arginfo_saxonc_xdmarray_values,
    // ZEND_ACC_PUBLIC)
    PHP_ME(XdmArray, getArity, arginfo_saxonc_xdmarray_getArity,
           ZEND_ACC_PUBLIC)
        PHP_ME(XdmArray, arrayLength, arginfo_saxonc_xdmarray_arrayLength,
               ZEND_ACC_PUBLIC){nullptr, nullptr, nullptr}};

PHP_MINIT_FUNCTION(saxon) {

  zend_class_entry ce;
  INIT_CLASS_ENTRY(ce, "Saxon\\SaxonProcessor", SaxonProcessor_methods);
  saxonProcessor_ce = zend_register_internal_class(&ce);
  saxonProcessor_ce->create_object = saxonProcessor_create_handler;
  memcpy(&saxonProcessor_object_handlers, zend_get_std_object_handlers(),
         sizeof(saxonProcessor_object_handlers)); // zend_object_handlers
  saxonProcessor_object_handlers.offset =
      XtOffsetOf(saxonProcessor_object, std);
  saxonProcessor_object_handlers.free_obj = SaxonProcessor_free_storage;
  saxonProcessor_object_handlers.dtor_obj = SaxonProcessor_destroy_storage;

  // saxonProcessor_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\DocumentBuilder", DocumentBuilder_methods);
  documentBuilder_ce = zend_register_internal_class(&ce);
  documentBuilder_ce->create_object = documentBuilder_create_handler;
  memcpy(&documentBuilder_object_handlers, zend_get_std_object_handlers(),
         sizeof(documentBuilder_object_handlers));
  documentBuilder_object_handlers.offset =
      XtOffsetOf(documentBuilder_object, std);
  documentBuilder_object_handlers.free_obj = DocumentBuilder_free_storage;
  documentBuilder_object_handlers.dtor_obj = DocumentBuilder_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\Xslt30Processor", Xslt30Processor_methods);
  xslt30Processor_ce = zend_register_internal_class(&ce);
  xslt30Processor_ce->create_object = xslt30Processor_create_handler;
  memcpy(&xslt30Processor_object_handlers, zend_get_std_object_handlers(),
         sizeof(xslt30Processor_object_handlers));
  xslt30Processor_object_handlers.offset =
      XtOffsetOf(xslt30Processor_object, std);
  xslt30Processor_object_handlers.free_obj = Xslt30Processor_free_storage;
  xslt30Processor_object_handlers.dtor_obj = Xslt30Processor_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XsltExecutable", XsltExecutable_methods);
  xsltExecutable_ce = zend_register_internal_class(&ce);
  xsltExecutable_ce->create_object = xslt30Processor_create_handler;
  memcpy(&xsltExecutable_object_handlers, zend_get_std_object_handlers(),
         sizeof(xsltExecutable_object_handlers));
  xsltExecutable_object_handlers.offset =
      XtOffsetOf(xsltExecutable_object, std);
  xsltExecutable_object_handlers.free_obj = XsltExecutable_free_storage;
  xsltExecutable_object_handlers.dtor_obj = XsltExecutable_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XQueryProcessor", XQueryProcessor_methods);
  xqueryProcessor_ce = zend_register_internal_class(&ce);
  xqueryProcessor_ce->create_object = xqueryProcessor_create_handler;
  memcpy(&xqueryProcessor_object_handlers, zend_get_std_object_handlers(),
         sizeof(xqueryProcessor_object_handlers));
  xqueryProcessor_object_handlers.offset =
      XtOffsetOf(xqueryProcessor_object, std);
  xqueryProcessor_object_handlers.free_obj = xqueryProcessor_free_storage;
  xqueryProcessor_object_handlers.dtor_obj = xqueryProcessor_destroy_storage;

  // xqueryProcessor_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\XPathProcessor", XPathProcessor_methods);
  xpathProcessor_ce = zend_register_internal_class(&ce);
  xpathProcessor_ce->create_object = xpathProcessor_create_handler;
  memcpy(&xpathProcessor_object_handlers, zend_get_std_object_handlers(),
         sizeof(xpathProcessor_object_handlers));
  xpathProcessor_object_handlers.offset =
      XtOffsetOf(xpathProcessor_object, std);
  xpathProcessor_object_handlers.free_obj = xpathProcessor_free_storage;
  xpathProcessor_object_handlers.dtor_obj = xpathProcessor_destroy_storage;
  // xpathProcessor_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\SchemaValidator", SchemaValidator_methods);
  schemaValidator_ce = zend_register_internal_class(&ce);
  schemaValidator_ce->create_object = schemaValidator_create_handler;
  memcpy(&schemaValidator_object_handlers, zend_get_std_object_handlers(),
         sizeof(schemaValidator_object_handlers));
  schemaValidator_object_handlers.offset =
      XtOffsetOf(schemaValidator_object, std);
  schemaValidator_object_handlers.free_obj = schemaValidator_free_storage;
  schemaValidator_object_handlers.dtor_obj = schemaValidator_destroy_storage;
  // schemaValidator_object_handlers.clone_obj = nullptr;

    INIT_CLASS_ENTRY(ce, "Saxon\\XsdCompiler", XsdCompiler_methods);
    xsdCompiler_ce = zend_register_internal_class(&ce);
    xsdCompiler_ce->create_object = xsdCompiler_create_handler;
    memcpy(&xsdCompiler_object_handlers, zend_get_std_object_handlers(),
           sizeof(xsdCompiler_object_handlers));
    xsdCompiler_object_handlers.offset =
        XtOffsetOf(xsdCompiler_object, std);
    xsdCompiler_object_handlers.free_obj = xsdCompiler_free_storage;
    xsdCompiler_object_handlers.dtor_obj = xsdCompiler_destroy_storage;

    INIT_CLASS_ENTRY(ce, "Saxon\\XsdSchema", XsdSchema_methods);
    xsdSchema_ce = zend_register_internal_class(&ce);
    xsdSchema_ce->create_object = xsdSchema_create_handler;
    memcpy(&xsdSchema_object_handlers, zend_get_std_object_handlers(),
           sizeof(xsdSchema_object_handlers));
    xsdSchema_object_handlers.offset =
        XtOffsetOf(xsdSchema_object, std);
    xsdSchema_object_handlers.free_obj = xsdSchema_free_storage;
    xsdSchema_object_handlers.dtor_obj = xsdSchema_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmValue", xdmValue_methods);
  xdmValue_ce = zend_register_internal_class(&ce);
  xdmValue_ce->create_object = xdmValue_create_handler;
  memcpy(&xdmValue_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmValue_object_handlers));
  xdmValue_object_handlers.offset = XtOffsetOf(xdmValue_object, std);
  xdmValue_object_handlers.free_obj = xdmValue_free_storage;
  xdmValue_object_handlers.dtor_obj = XdmValue_destroy_storage;
  // xdmValue_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmItem", xdmItem_methods);
  xdmItem_ce = zend_register_internal_class(&ce);
  xdmItem_ce->create_object = xdmItem_create_handler;
  memcpy(&xdmItem_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmItem_object_handlers));
  xdmItem_object_handlers.offset = XtOffsetOf(xdmItem_object, std);
  xdmItem_object_handlers.free_obj = xdmItem_free_storage;
  xdmItem_object_handlers.dtor_obj = XdmItem_destroy_storage;
  // xdmItem_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmNode", xdmNode_methods);
  xdmNode_ce = zend_register_internal_class(&ce);
  xdmNode_ce->create_object = xdmNode_create_handler;
  memcpy(&xdmNode_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmNode_object_handlers));
  xdmNode_object_handlers.offset = XtOffsetOf(xdmNode_object, std);
  xdmNode_object_handlers.free_obj = xdmNode_free_storage;
  xdmNode_object_handlers.dtor_obj = XdmNode_destroy_storage;

  // xdmNode_object_handlers.clone_obj = nullptr;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmAtomicValue", xdmAtomicValue_methods);
  xdmAtomicValue_ce = zend_register_internal_class(&ce);
  xdmAtomicValue_ce->create_object = xdmAtomicValue_create_handler;
  memcpy(&xdmAtomicValue_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmAtomicValue_object_handlers));
  xdmAtomicValue_object_handlers.offset =
      XtOffsetOf(xdmAtomicValue_object, std);
  xdmAtomicValue_object_handlers.free_obj = xdmAtomicValue_free_storage;
  xdmAtomicValue_object_handlers.dtor_obj = XdmAtomicValue_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmFunctionItem", xdmFunctionItem_methods);
  xdmFunctionItem_ce = zend_register_internal_class(&ce);
  xdmFunctionItem_ce->create_object = xdmFunctionItem_create_handler;
  memcpy(&xdmFunctionItem_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmFunctionItem_object_handlers));
  xdmFunctionItem_object_handlers.offset =
      XtOffsetOf(xdmFunctionItem_object, std);
  xdmFunctionItem_object_handlers.free_obj = xdmFunctionItem_free_storage;
  xdmFunctionItem_object_handlers.dtor_obj = XdmFunctionItem_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmMap", xdmMap_methods);
  xdmMap_ce = zend_register_internal_class(&ce);
  xdmMap_ce->create_object = xdmMap_create_handler;
  memcpy(&xdmMap_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmMap_object_handlers));
  xdmMap_object_handlers.offset = XtOffsetOf(xdmMap_object, std);
  xdmMap_object_handlers.free_obj = xdmMap_free_storage;
  xdmMap_object_handlers.dtor_obj = XdmMap_destroy_storage;

  INIT_CLASS_ENTRY(ce, "Saxon\\XdmArray", xdmArray_methods);
  xdmArray_ce = zend_register_internal_class(&ce);
  xdmArray_ce->create_object = xdmArray_create_handler;
  memcpy(&xdmArray_object_handlers, zend_get_std_object_handlers(),
         sizeof(xdmArray_object_handlers));
  xdmArray_object_handlers.offset = XtOffsetOf(xdmArray_object, std);
  xdmArray_object_handlers.free_obj = xdmArray_free_storage;
  xdmArray_object_handlers.dtor_obj = XdmArray_destroy_storage;

  // REGISTER_INI_ENTRIES();
  return SUCCESS;
}

PHP_MINFO_FUNCTION(saxon) {
  php_info_print_table_start();
  php_info_print_table_header(2, "saxonc", "enabled");
  php_info_print_table_row(2, "SaxonC API version", "13.0.0");
  php_info_print_table_end();
  DISPLAY_INI_ENTRIES();
}

PHP_MSHUTDOWN_FUNCTION(saxon) {
  // std::cerr<<"MSHUTDOWN called -start"<<std::endl;
  // SaxonProcessor *  proc = new SaxonProcessor();
  SaxonProcessor::release();
  // delete proc;
  UNREGISTER_INI_ENTRIES();

  return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(saxon) {
  // std::cerr<<"RSHUTDOWN called -start"<<std::endl;
  //    SaxonProcessor::release();
  return SUCCESS;
}

PHP_RINIT_FUNCTION(saxon) {
  // SaxonProcessor::release();
  // std::cerr<<"RINIT called -start"<<std::endl;
  return SUCCESS;
}

zend_module_entry saxon_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SAXON_EXTNAME,
    nullptr,              /* Functions */
    PHP_MINIT(saxon),     /* MINIT */
    PHP_MSHUTDOWN(saxon), /* MSHUTDOWN */
    nullptr,              /* RINIT */
    nullptr,              /* RSHUTDOWN */
    PHP_MINFO(saxon),     /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SAXON_EXTVER,
#endif
    STANDARD_MODULE_PROPERTIES};

#ifdef COMPILE_DL_SAXONC
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(saxonc)
#endif
