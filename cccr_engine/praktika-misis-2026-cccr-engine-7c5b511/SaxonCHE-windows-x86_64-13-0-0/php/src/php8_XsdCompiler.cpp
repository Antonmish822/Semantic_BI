zend_object_handlers xsdCompiler_object_handlers;
zend_class_entry *xsdCompiler_ce;

/*     ============== PHP Interface of   XsdCompiler_object =============== */

void xsdCompiler_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

void xsdCompiler_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xsdCompiler_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  xsdCompiler_object *obj = (xsdCompiler_object *)ecalloc(
      1, sizeof(xsdCompiler_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std, type);
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xsdCompiler_object_handlers;

  return &obj->std;
}

PHP_METHOD(XsdCompiler, __destruct) {
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));

  XsdCompiler *xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    xsdCompiler->clearProperties();
    delete xsdCompiler;
  }
}

PHP_METHOD(XsdCompiler, setXsdVersion){
  XsdCompiler *xsdCompiler;
  char *verStr;
  size_t len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &verStr, &len) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {

    if (verStr != nullptr) {
      xsdCompiler->setXsdVersion(verStr);
    }
  }
}


PHP_METHOD(XsdCompiler, getXsdVersion){
  XsdCompiler *xsdCompiler;

    if (ZEND_NUM_ARGS() > 0) {
      WRONG_PARAM_COUNT;
    }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {

    const char * verStr = xsdCompiler->getXsdVersion();
    if(verStr != nullptr) {
      RETURN_STRING(verStr);
    } else {
      RETURN_NULL();
    }
  }
}


PHP_METHOD(XsdCompiler, compileFromString){
  XsdCompiler *xsdCompiler;
  char *schemaStr;
  char *encodingStr = nullptr;
  size_t len1, len2;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &schemaStr, &len1) ==
          FAILURE) {
    RETURN_NULL();
  } else if (ZEND_NUM_ARGS() == 2 &&
             zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &schemaStr, &len1,
                                   &encodingStr, &len2) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    try {
      XsdSchema * schema =
          xsdCompiler->compileFromString(schemaStr, encodingStr);

      if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *ooth = Z_OBJ_P(return_value);
        xsdSchema_object *vobj =
            (xsdSchema_object *)((char *)ooth -
                                      XtOffsetOf(xsdSchema_object, std));

        assert(vobj != nullptr);

        vobj->xsdSchema = schema;
      }

    } catch (SaxonApiException &e) {
      zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XsdCompiler, compileFromFile){
 XsdCompiler *xsdCompiler;
  char *schemaFilename;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &schemaFilename, &len1) ==
          FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    try {
      XsdSchema * schema =
          xsdCompiler->compileFromFile(schemaFilename);

      if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *ooth = Z_OBJ_P(return_value);
        xsdSchema_object *vobj =
            (xsdSchema_object *)((char *)ooth -
                                      XtOffsetOf(xsdSchema_object, std));

        assert(vobj != nullptr);
        vobj->xsdSchema = schema;
      }

    } catch (SaxonApiException &e) {
      zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
  } else {
    RETURN_NULL();
  }
}


PHP_METHOD(XsdCompiler, compileFromNode){
  XsdCompiler *xsdCompiler;
  zval *oth;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &oth, xdmNode_ce) ==
      FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    zend_object *vvobj = Z_OBJ_P(oth);
    xdmNode_object *ooth =
        (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));
    if (ooth != nullptr) {
      XdmNode *value = ooth->xdmNode;
      if (value != nullptr) {
        try {
          XsdSchema *schema = xsdCompiler->compileFromNode(value);
          if (schema != nullptr) {
            if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
              RETURN_NULL();
            } else {
              zend_object *ooth = Z_OBJ_P(return_value);
              xsdSchema_object *vobj =
                  (xsdSchema_object *)((char *)ooth -
                                            XtOffsetOf(xsdSchema_object,
                                                       std));
              assert(vobj != nullptr);
              vobj->xsdSchema = schema;
            }
          }
        } catch (SaxonApiException &e) {
          zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(),
                               0);
        }
      } else {
        zend_throw_exception(zend_ce_exception, "Argument must contain array of XdmNode objects",
                             0);
      }
    }
  }
}


PHP_METHOD(XsdCompiler, combine){
 XsdCompiler *xsdCompiler = nullptr;
  HashTable *arr_hash;

  zval *items_zval;
  zval *val;
  long num_key;
  XsdSchema **arguments = nullptr;
  int argument_length = 0;
  zend_string *key;

  if (ZEND_NUM_ARGS() != 1) {
    WRONG_PARAM_COUNT;
  }

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &items_zval) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                XtOffsetOf(xsdCompiler_object, std));

  xsdCompiler = obj->xsdCompiler;
  arr_hash = Z_ARRVAL_P(items_zval);
  argument_length = zend_hash_num_elements(arr_hash);

  arguments = new XsdSchema *[argument_length];

  if (xsdCompiler != nullptr) {
    ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {
      if (Z_TYPE_P(val) != IS_NULL) {
        const char *objName = ZSTR_VAL(Z_OBJCE_P(val)->name);

        if (strcmp(objName, "Saxon\\XsdSchema") == 0) {
          zend_object *ooth = Z_OBJ_P(val);
          xsdSchema_object *nobj =
              (xsdSchema_object *)((char *)ooth -
                                 XtOffsetOf(xsdSchema_object, std));
          if (nobj != nullptr) {
            XsdSchema *value = (XsdSchema *)nobj->xsdSchema;
            arguments[num_key] = value;
          }
        } else  {
          zend_throw_exception(zend_ce_exception, "Expecting XsdSchema object but found another object type", 0);
        }
      } else {
        // TODO error warning
      }
    }
    ZEND_HASH_FOREACH_END();

    XsdSchema *xsdSchema = xsdCompiler->combine(arguments, argument_length);

    if (xsdSchema != nullptr) {

      if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
        RETURN_NULL();
      } else {
        zend_object *vvobj = Z_OBJ_P(return_value);
        xsdSchema_object *vobj =
            (xsdSchema_object *)((char *)vvobj -
                                XtOffsetOf(xsdSchema_object, std));
        assert(vobj != nullptr);
        vobj->xsdSchema = xsdSchema;
        return;
      }
    } else {
      RETURN_NULL();
    }
  }
}

PHP_METHOD(XsdCompiler, emptySchema){
  XsdCompiler *xsdCompiler;

    if (ZEND_NUM_ARGS() > 0) {
      WRONG_PARAM_COUNT;
    }
 zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    try {
      XsdSchema * schema =
          xsdCompiler->emptySchema();

      if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *ooth = Z_OBJ_P(return_value);
        xsdSchema_object *vobj =
            (xsdSchema_object *)((char *)ooth -
                                      XtOffsetOf(xsdSchema_object, std));

        assert(vobj != nullptr);

        vobj->xsdSchema = schema;
      }

    } catch (SaxonApiException &e) {
      zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XsdCompiler, importComponents){
XsdCompiler *xsdCompiler;
  char *schemaFilename;
  size_t len1;

  if (ZEND_NUM_ARGS() == 1 &&
      zend_parse_parameters(ZEND_NUM_ARGS(), "s", &schemaFilename, &len1) ==
          FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    try {
      XsdSchema * schema =
          xsdCompiler->importComponents(schemaFilename);

      if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
        RETURN_NULL();
      } else {

        zend_object *ooth = Z_OBJ_P(return_value);
        xsdSchema_object *vobj =
            (xsdSchema_object *)((char *)ooth -
                                      XtOffsetOf(xsdSchema_object, std));

        assert(vobj != nullptr);
        vobj->xsdSchema = schema;
      }

    } catch (SaxonApiException &e) {
      zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
  } else {
    RETURN_NULL();
  }
}

PHP_METHOD(XsdCompiler, importComponentsFromNode){
 XsdCompiler *xsdCompiler;
  zval *oth;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &oth, xdmNode_ce) ==
      FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    zend_object *vvobj = Z_OBJ_P(oth);
    xdmNode_object *ooth =
        (xdmNode_object *)((char *)vvobj - XtOffsetOf(xdmNode_object, std));
    if (ooth != nullptr) {
      XdmNode *value = ooth->xdmNode;
      if (value != nullptr) {
        try {
          XsdSchema *schema = xsdCompiler->importComponentsFromNode(value);
          if (schema != nullptr) {
            if (object_init_ex(return_value, xsdSchema_ce) != SUCCESS) {
              RETURN_NULL();
            } else {
              zend_object *ooth = Z_OBJ_P(return_value);
              xsdSchema_object *vobj =
                  (xsdSchema_object *)((char *)ooth -
                                            XtOffsetOf(xsdSchema_object,
                                                       std));
              assert(vobj != nullptr);
              vobj->xsdSchema = schema;
            }
          }
        } catch (SaxonApiException &e) {
          zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(),
                               0);
        }
      } else {
        zend_throw_exception(zend_ce_exception, "XdmValue is not an XdmNode",0);
      }
    }
  }
}

PHP_METHOD(XsdCompiler, setProperty){
  XsdCompiler *xsdCompiler;
  char *name;
  char *value;
  size_t len1, len2, myint;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &name, &len1, &value,
                            &len2) == FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    xsdCompiler->setProperty(name, value);
  }
}

PHP_METHOD(XsdCompiler, clearProperties){
  XsdCompiler *xsdCompiler;
  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {
    xsdCompiler->clearProperties();
  }
}

PHP_METHOD(XsdCompiler, setcwd){
  XsdCompiler *xsdCompiler;
  char *cwdStr;
  size_t len;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &cwdStr, &len) == FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {

    if (cwdStr != nullptr) {
      xsdCompiler->setcwd(cwdStr);
    }
  }
}

PHP_METHOD(XsdCompiler, getcwd){
  XsdCompiler *xsdCompiler;

    if (ZEND_NUM_ARGS() > 0) {
      WRONG_PARAM_COUNT;
    }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdCompiler_object *obj =
      (xsdCompiler_object *)((char *)pobj -
                                 XtOffsetOf(xsdCompiler_object, std));
  xsdCompiler = obj->xsdCompiler;
  if (xsdCompiler != nullptr) {

    const char * cwdStr = xsdCompiler->getcwd();
    if(cwdStr != nullptr) {
      RETURN_STRING(cwdStr);
    } else {
      RETURN_NULL();
    }
  }
}



zend_function_entry XsdCompiler_methods[] = {
    PHP_ME(
        XsdCompiler, __destruct, arginfo_saxonc_xsdCompiler_destruct,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, setXsdVersion,arginfo_saxonc_xsdCompiler_setXsdVersion,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler,getXsdVersion,arginfo_saxonc_xsdCompiler_getXsdVersion,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, compileFromString, arginfo_saxonc_xsdCompiler_compileFromString,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, compileFromFile,arginfo_saxonc_xsdCompiler_compileFromFile,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, compileFromNode,arginfo_saxonc_xsdCompiler_compileFromNode,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler,combine,arginfo_saxonc_xsdCompiler_combine,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, emptySchema,arginfo_saxonc_xsdCompiler_emptySchema,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, importComponents,arginfo_saxonc_xsdCompiler_importComponents,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, importComponentsFromNode,arginfo_saxonc_xsdCompiler_importComponentsFromNode,ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, setProperty,arginfo_saxonc_xsdCompiler_setProperty, ZEND_ACC_PUBLIC)
        PHP_ME(XsdCompiler, clearProperties,arginfo_saxonc_xsdCompiler_clearProperties,ZEND_ACC_PUBLIC)
    {nullptr, nullptr, nullptr}};