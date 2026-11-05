zend_object_handlers xsdSchema_object_handlers;

/*     ============== PHP Interface of   xsdSchema_object =============== */

void xsdSchema_free_storage(zend_object *object) {
  zend_object_std_dtor(object);
}

void xsdSchema_destroy_storage(zend_object *object) {
  zend_objects_destroy_object(object);
}

zend_object *xsdSchema_create_handler(zend_class_entry *type) {
  zval *tmp;
  zend_object retval;
  xsdSchema_object *obj = (xsdSchema_object *)ecalloc(
      1, sizeof(xsdSchema_object) + zend_object_properties_size(type));

  zend_object_std_init(&obj->std, type);
  object_properties_init(&obj->std, type);

  obj->std.handlers = &xsdSchema_object_handlers;

  return &obj->std;
}

PHP_METHOD(XsdSchema, __destruct) {
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdSchema_object *obj =
      (xsdSchema_object *)((char *)pobj -
                                 XtOffsetOf(xsdSchema_object, std));

  XsdSchema *xsdSchema = obj->xsdSchema;
  if (xsdSchema != nullptr) {
    delete xsdSchema;
  }
}

PHP_METHOD(XsdSchema, newValidator) {

  if (ZEND_NUM_ARGS() > 0) {
    WRONG_PARAM_COUNT;
  }
  XsdSchema * xsdSchema = nullptr;
  SchemaValidator * schemaValidator = nullptr;

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdSchema_object *obj =
      (xsdSchema_object *)((char *)pobj -
                                XtOffsetOf(xsdSchema_object, std));
  assert(obj != nullptr);
  xsdSchema = obj->xsdSchema;
  assert(xsdSchema != nullptr);
  if (xsdSchema != nullptr) {
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
        schemaValidator = xsdSchema->newValidator();
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

PHP_METHOD(XsdSchema, combine) {
 XsdSchema *xsdSchema;
  zval *oth;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &oth, xsdSchema_ce) ==
      FAILURE) {
    RETURN_NULL();
  }
  zend_object *pobj = Z_OBJ_P(getThis());
  xsdSchema_object *obj =
      (xsdSchema_object *)((char *)pobj -
                                 XtOffsetOf(xsdSchema_object, std));
  xsdSchema = obj->xsdSchema;
  if (xsdSchema != nullptr) {
    zend_object *vvobj = Z_OBJ_P(oth);
    xsdSchema_object *ooth =
        (xsdSchema_object *)((char *)vvobj - XtOffsetOf(xsdSchema_object, std));
    if (ooth != nullptr) {
      XsdSchema *value = ooth->xsdSchema;
      if (value != nullptr) {
        try {
          XsdSchema *schema = xsdSchema->combine(value);
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
        zend_throw_exception(zend_ce_exception, "Argument must contain XdmSchema object",
                             0);
      }
    }
  }
}


PHP_METHOD(XsdSchema, exportComponents) {
 XsdSchema * xsdSchema;
  char *filename;
  size_t len1;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &filename, &len1) ==
          FAILURE) {
    RETURN_NULL();
  }

  zend_object *pobj = Z_OBJ_P(getThis());
  xsdSchema_object *obj =
      (xsdSchema_object *)((char *)pobj -
                                 XtOffsetOf(xsdSchema_object, std));
  xsdSchema = obj->xsdSchema;
  if (xsdSchema != nullptr) {
    try {
      xsdSchema->exportComponents(filename);
    } catch (SaxonApiException &e) {
      zend_throw_exception(zend_ce_exception, e.getMessageWithErrorCode(), 0);
    }
  }
}



zend_function_entry XsdSchema_methods[] = {
    PHP_ME(XsdSchema, __destruct, arginfo_saxonc_xsdSchema_destruct,ZEND_ACC_PUBLIC)
    PHP_ME(XsdSchema, newValidator,arginfo_saxonc_xsdSchema_newValidator,ZEND_ACC_PUBLIC)
    PHP_ME(XsdSchema,combine,arginfo_saxonc_xsdSchema_combine,ZEND_ACC_PUBLIC)
    PHP_ME(XsdSchema, exportComponents, arginfo_saxonc_xsdSchema_exportComponents,ZEND_ACC_PUBLIC)
    {nullptr, nullptr, nullptr}};
