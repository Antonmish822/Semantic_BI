#ifndef __SAXONC_CORE_HE_H
#define __SAXONC_CORE_HE_H

#include <graal_isolate_dynamic.h>


#if defined(__cplusplus)
extern "C" {
#endif

typedef long long int (*createDocumentBuilder_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_db_setBaseURI_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef long long int (*j_db_setLineNumbering_fn_t)(graal_isolatethread_t*, void *, int);

typedef long long int (*j_db_setDTDValidation_fn_t)(graal_isolatethread_t*, void *, int);

typedef long long int (*j_xsltExecutable_messageHandler_file_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*j_create_mapData_fn_t)(graal_isolatethread_t*);

typedef long long int (*j_create_mapDataWithCapacity_fn_t)(graal_isolatethread_t*, int);

typedef long long int (*j_addMapPair_fn_t)(graal_isolatethread_t*, void *, void *, void *);

typedef long long int (*j_makeXdmMap_fn_t)(graal_isolatethread_t*, void *);

typedef long long* (*j_getXslMessages_fn_t)(graal_isolatethread_t*, void *, void *);

typedef void (*j_clearXslMessages_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xsltExecutable_messageHandler_null_fn_t)(graal_isolatethread_t*, saxon_error_t*);

typedef long long int (*createProcessorDataWithArray_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*createProcessorData_fn_t)(graal_isolatethread_t*);

typedef long long int (*createProcessorDataWithCapacity_fn_t)(graal_isolatethread_t*, int);

typedef void (*addProcessorDataPair_fn_t)(graal_isolatethread_t*, void *, char*, void *, saxon_error_t*);

typedef void (*addProcessorPropertyPair_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef void (*addProcessorValue_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getProcessorDataProperty_fn_t)(graal_isolatethread_t*, void *, void *, int);

typedef void (*addProcessorProperty_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef void (*clearProcessorData_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_getQName_fn_t)(graal_isolatethread_t*, char*, char*, char*, saxon_error_t*);

typedef long long int (*j_getQNameFromLocal_fn_t)(graal_isolatethread_t*, char*, saxon_error_t*);

typedef long long int (*j_getQNameFromNode_fn_t)(graal_isolatethread_t*, char*, void *, saxon_error_t*);

typedef long long int (*j_getQNameFromLexical_fn_t)(graal_isolatethread_t*, char*, char*, saxon_error_t*);

typedef char (*j_isQNameEqual_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_fromClarkName_fn_t)(graal_isolatethread_t*, char*, saxon_error_t*);

typedef long long int (*j_fromEQName_fn_t)(graal_isolatethread_t*, char*, saxon_error_t*);

typedef char (*j_isQNameValid_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef char* (*j_getPrefixFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getNamespaceFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getNamespaceUriFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getLocalNameFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getClarkNameFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getEQNameFromQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_toStringQName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_getArgumentTypesFromSignature_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_getReturnTypeFromSignature_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_setPHPNative_fn_t)(graal_isolatethread_t*);

typedef long long int (*j_setPythonNative_fn_t)(graal_isolatethread_t*);

typedef long long int (*j_setIndirectFunctionForNativeCall_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_setIndirectFunctionForPythonNativeCall_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_registerFunctionDefinition_fn_t)(graal_isolatethread_t*, void *, char*, void *, void *, void *, void *, int, saxon_error_t*);

typedef long long int (*j_setLibrary_fn_t)(graal_isolatethread_t*, void *, char*, char*);

typedef long long int (*createSaxonProcessor_fn_t)(graal_isolatethread_t*, int);

typedef void (*j_gc_fn_t)(graal_isolatethread_t*);

typedef char (*j_isSchemaAware_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef char (*j_isLicensed_fn_t)(graal_isolatethread_t*, void *);

typedef char* (*j_c_getSaxonEdition_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_run_transform_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_run_gizmo_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_run_query_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_copy_handles_fn_t)(graal_isolatethread_t*, void *);

typedef void (*j_handles_destroy_fn_t)(graal_isolatethread_t*, void *);

typedef void (*j_printObjectHandlesPoolInfo_fn_t)(graal_isolatethread_t*);

typedef void (*j_createHeapDump_fn_t)(graal_isolatethread_t*, int);

typedef long long* (*j_getLongArray_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_createStringObject_fn_t)(graal_isolatethread_t*, char*, saxon_error_t*);

typedef long long int (*createSaxonProcessorWithConfigurationFile_fn_t)(graal_isolatethread_t*, char*, saxon_error_t*);

typedef char* (*j_c_getProductVersion_fn_t)(graal_isolatethread_t*, void *);

typedef char* (*j_cpp_getProductVersion_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_setCatalogFiles_fn_t)(graal_isolatethread_t*, void *, char*, void *, saxon_error_t*);

typedef long long int (*j_applyPropertyToConfiguration_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*j_applyPropertiesToConfiguration_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef void (*setDebugMode_fn_t)(graal_isolatethread_t*, int);

typedef long long int (*parseJsonFile_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*parseJsonString_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*parseXmlFileWithValidator_fn_t)(graal_isolatethread_t*, char*, void *, void *, char*, saxon_error_t*);

typedef long long int (*parseXmlString_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*parseXmlStringWithValidator_fn_t)(graal_isolatethread_t*, void *, char*, void *, char*, saxon_error_t*);

typedef long long int (*createXdmAtomicItem_fn_t)(graal_isolatethread_t*, char*, char*, saxon_error_t*);

typedef long long int (*j_main_parseXmlString_fn_t)(graal_isolatethread_t*, char*, void *, void *, void *, char*, char*, saxon_error_t*);

typedef char* (*getTypeName_fn_t)(graal_isolatethread_t*, char*);

typedef char* (*j_getStringValue_fn_t)(graal_isolatethread_t*, void *);

typedef char* (*j_cpp_getStringValue_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_cpp_encodeString_fn_t)(graal_isolatethread_t*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_main_parseXmlFile_fn_t)(graal_isolatethread_t*, char*, void *, void *, void *, char*, saxon_error_t*);

typedef long long int (*createSaxonCResultDocumentHandler_fn_t)(graal_isolatethread_t*, int);

typedef long long* (*j_getResultDocuments_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_getResultDocumentURIs_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*createSchemaValidator_fn_t)(graal_isolatethread_t*, saxon_error_t*);

typedef long long int (*j_create_sxn_XsdCompiler_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*createSchemaValidatorWithProcessor_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_getSchemaManager_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_getValidationReport_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_registerSchema_fn_t)(graal_isolatethread_t*, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_registerSchemaString_fn_t)(graal_isolatethread_t*, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_registerSchemaNode_fn_t)(graal_isolatethread_t*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_exportSchema_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*j_validateFromString_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_validate_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_validateToNode_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_xsltExecutable_messageHandler_store_fn_t)(graal_isolatethread_t*, saxon_error_t*);

typedef long long int (*createXPathProcessor_fn_t)(graal_isolatethread_t*);

typedef long long int (*createXPathProcessorWithProcessor_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xp_declareNamespace_fn_t)(graal_isolatethread_t*, void *, char*, char*, saxon_error_t*);

typedef long long int (*j_xp_declareVariable_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef long long int (*j_xp_importSchemaNamespace_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef void (*j_xp_setAllowUndeclaredVariables_fn_t)(graal_isolatethread_t*, void *, int);

typedef void (*j_xp_setBackwardsCompatible_fn_t)(graal_isolatethread_t*, void *, int);

typedef long long int (*j_xp_setBaseURI_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef char* (*j_xp_getBaseURI_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_xp_useSchema_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_evaluate_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_evaluateSingle_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef int (*j_effectiveBooleanValue_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_createXQueryEngine_fn_t)(graal_isolatethread_t*);

typedef long long int (*j_createXQueryEngineWithProcessor_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef char* (*j_executeQueryToString_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, char*, void *, saxon_error_t*);

typedef char* (*j_executeQueryToString_c_fn_t)(graal_isolatethread_t*, void *, char*, void *, saxon_error_t*);

typedef long long int (*j_executeQueryToValue_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_executeQueryToFile_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, char*, void *, saxon_error_t*);

typedef char* (*getBaseURIForXdmNode_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*getParentForXdmNode_fn_t)(graal_isolatethread_t*, void *);

typedef char* (*getStringValueForXdmValue_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char* (*getStringValueForXdmItem_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char* (*xdmItemToString_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char* (*j_EQNameToClarkName_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef char* (*j_clarkNameToEQName_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef char* (*xdmNodeToString_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char* (*xdmMapToString_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char (*j_xdmNodeEquals_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef char* (*j_getPrimitiveTypeName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getFunctionName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef int (*j_xdmFunctionItem_getArity_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xdmFunctionItem_call_fn_t)(graal_isolatethread_t*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_getSystemFunction_fn_t)(graal_isolatethread_t*, void *, char*, int, saxon_error_t*);

typedef int (*j_xdmArray_arrayLength_fn_t)(graal_isolatethread_t*, void *);

typedef int (*j_xdmMap_size_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xdmArray_get_fn_t)(graal_isolatethread_t*, void *, int, saxon_error_t*);

typedef long long int (*j_xdmArray_put_fn_t)(graal_isolatethread_t*, void *, int, void *, saxon_error_t*);

typedef long long int (*j_xdmMap_remove_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_xdmArray_addMember_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_xdmArray_concat_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef char* (*j_xdmArray_toString_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long* (*j_convertXdmArrayToArrayObject_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_makeXdmArray_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_makeXdmArrayFromShort_fn_t)(graal_isolatethread_t*, short*, int);

typedef long long int (*j_makeXdmArrayFromStrings_fn_t)(graal_isolatethread_t*, char**, int, saxon_error_t*);

typedef long long int (*j_makeXdmArrayFromInt_fn_t)(graal_isolatethread_t*, int*, int);

typedef long long int (*j_makeXdmArrayFromLong_fn_t)(graal_isolatethread_t*, long long*, int);

typedef long long int (*j_makeXdmArrayFromBool_fn_t)(graal_isolatethread_t*, int*, int);

typedef long long* (*j_xdmMap_values_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef char (*j_xdmMap_containsKey_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char (*j_xdmMap_isEmpty_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xdmMap_put_fn_t)(graal_isolatethread_t*, void *, void *, void *);

typedef long long int (*j_xdmMap_get_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_xdmMap_get_with_key_as_string_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef long long int (*j_xdmMap_get_with_key_as_int_fn_t)(graal_isolatethread_t*, void *, int, saxon_error_t*);

typedef long long int (*j_xdmMap_get_with_key_as_double_fn_t)(graal_isolatethread_t*, void *, double, saxon_error_t*);

typedef long long int (*j_xdmMap_get_with_key_as_long_fn_t)(graal_isolatethread_t*, void *, long long int, saxon_error_t*);

typedef long long* (*j_xdmMap_keys_fn_t)(graal_isolatethread_t*, void *, void *);

typedef int (*j_getNodeKind_fn_t)(graal_isolatethread_t*, void *);

typedef char* (*j_getLocalName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*j_getNodeName_fn_t)(graal_isolatethread_t*, void *, void *);

typedef char* (*xdmValueArrayToString_fn_t)(graal_isolatethread_t*, void *, void *, char*, saxon_error_t*);

typedef char* (*j_getAttributeValue_fn_t)(graal_isolatethread_t*, void *, char*, void *, saxon_error_t*);

typedef long long* (*j_getAttributeNodes_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long* (*j_getChildren_fn_t)(graal_isolatethread_t*, void *, void *);

typedef int (*j_getXdmObjectType_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_getChild_fn_t)(graal_isolatethread_t*, void *, int);

typedef int (*j_getChildCount_fn_t)(graal_isolatethread_t*, void *);

typedef int (*j_getAttributeCount_fn_t)(graal_isolatethread_t*, void *);

typedef long long* (*j_axisIterator_fn_t)(graal_isolatethread_t*, void *, void *, int);

typedef long long int (*j_makeStringValue_fn_t)(graal_isolatethread_t*, char*, char*, saxon_error_t*);

typedef long long int (*j_makeIntegerValue_fn_t)(graal_isolatethread_t*, int);

typedef long long int (*j_makeDoubleValue_fn_t)(graal_isolatethread_t*, double);

typedef long long int (*j_makeFloatValue_fn_t)(graal_isolatethread_t*, float);

typedef long long int (*j_makeLongValue_fn_t)(graal_isolatethread_t*, long long int);

typedef long long int (*j_makeBooleanValue_fn_t)(graal_isolatethread_t*, int);

typedef long long int (*j_makeQNameValue_fn_t)(graal_isolatethread_t*, char*);

typedef long long int (*j_makeAtomicValue_fn_t)(graal_isolatethread_t*, char*, char*, saxon_error_t*);

typedef long long* (*makeArrayFromXdmValue2_fn_t)(graal_isolatethread_t*, void *, void *);

typedef long long int (*j_getTypedValue_fn_t)(graal_isolatethread_t*, void *);

typedef int (*j_getLineNumber1_fn_t)(graal_isolatethread_t*, void *);

typedef int (*j_getColumnNumber1_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_getXdmEmptySequence_fn_t)(graal_isolatethread_t*);

typedef char (*j_getBooleanValue_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef double (*j_getDoubleValue_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_getLongValue_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef int (*j_getHashCode_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_createXsdCompiler_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_xsdCompileFromString_fn_t)(graal_isolatethread_t*, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_xsdCompileFromNode_fn_t)(graal_isolatethread_t*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_xsdempty_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xsdcombine_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_xsdCompileFromFile_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_xsdCompileFromProcessorData_fn_t)(graal_isolatethread_t*, void *, void *, char*, void *, void *, saxon_error_t*);

typedef long long int (*j_xsdImportComponentsFromNode_fn_t)(graal_isolatethread_t*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_xsdImportComponents_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_newSchemaValidator_fn_t)(graal_isolatethread_t*, void *);

typedef long long int (*j_xsd_schema_combine_fn_t)(graal_isolatethread_t*, void *, void *, saxon_error_t*);

typedef long long int (*j_exportComponentsToFile_fn_t)(graal_isolatethread_t*, void *, char*, saxon_error_t*);

typedef long long int (*createXslt30Processor_fn_t)(graal_isolatethread_t*);

typedef long long int (*createXslt30WithProcessor_fn_t)(graal_isolatethread_t*, void *, saxon_error_t*);

typedef long long int (*j_compileFromFileAndSave_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_compileFromFileAndImport_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_save_fn_t)(graal_isolatethread_t*, char*, void *, char*, saxon_error_t*);

typedef long long int (*j_compileFromStringAndSave_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_compileFromXdmNodeAndSave_fn_t)(graal_isolatethread_t*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_compileFromAssociatedFile_fn_t)(graal_isolatethread_t*, void *, char*, char*, int, void *, saxon_error_t*);

typedef long long int (*j_compileFromFile_fn_t)(graal_isolatethread_t*, void *, char*, char*, int, void *, saxon_error_t*);

typedef long long int (*j_compileFromString_fn_t)(graal_isolatethread_t*, void *, char*, char*, int, void *, saxon_error_t*);

typedef long long int (*j_compileFromXdmNode_fn_t)(graal_isolatethread_t*, void *, char*, void *, int, void *, saxon_error_t*);

typedef long long int (*j_applyTemplatesReturningValue_fn_t)(graal_isolatethread_t*, char*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_applyTemplatesReturningFile_fn_t)(graal_isolatethread_t*, char*, void *, void *, char*, void *, saxon_error_t*);

typedef char* (*j_applyTemplatesReturningString_fn_t)(graal_isolatethread_t*, void *, char*, void *, void *, void *, saxon_error_t*);

typedef long long int (*j_callFunctionReturningFile_fn_t)(graal_isolatethread_t*, char*, void *, char*, char*, void *, void *, saxon_error_t*);

typedef long long int (*j_callFunctionReturningValue_fn_t)(graal_isolatethread_t*, char*, void *, char*, void *, void *, saxon_error_t*);

typedef char* (*j_callFunctionReturningString_fn_t)(graal_isolatethread_t*, void *, char*, void *, char*, void *, void *, saxon_error_t*);

typedef long long int (*j_callTemplateReturningFile_fn_t)(graal_isolatethread_t*, char*, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_callTemplateReturningValue_fn_t)(graal_isolatethread_t*, char*, void *, char*, void *, saxon_error_t*);

typedef char* (*j_callTemplateReturningString_fn_t)(graal_isolatethread_t*, void *, char*, void *, char*, void *, saxon_error_t*);

typedef long long int (*j_transformToFile_fn_t)(graal_isolatethread_t*, char*, void *, void *, char*, char*, char*, void *, saxon_error_t*);

typedef long long int (*j_transformToValue_fn_t)(graal_isolatethread_t*, char*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef char* (*j_transformToString_fn_t)(graal_isolatethread_t*, void *, char*, void *, void *, char*, char*, void *, saxon_error_t*);

typedef long long int (*c_xsltSaveResultToFile_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, char*, void *, saxon_error_t*);

typedef char* (*c_xsltApplyStylesheet_fn_t)(graal_isolatethread_t*, void *, char*, char*, char*, void *, saxon_error_t*);

#if defined(__cplusplus)
}
#endif
#endif
