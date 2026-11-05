import os
import pathlib
from urllib.parse import urljoin, urlparse

import import_saxon


saxonc = import_saxon.import_mod()

optimizer_flags = '-'


class Spec:

    def __init__(self, v, fn, sspn, sav):
        self.version = v
        self.full_name = fn
        self.short_spec_name = sspn
        self.spec_and_version = sav


    @staticmethod
    def XP20():
        return Spec("2.0", "XPath2.0", "XP", "XP20")

    @staticmethod
    def XP30():
        return Spec("3.0", "XPath3.0", "XP", "XP30")

    @staticmethod
    def XP31():
        return Spec("3.1", "XPath3.1", "XP", "XP31")

    @staticmethod
    def XP40():
        return Spec("4.0", "XPath4.0", "XP", "XP40")

    @staticmethod
    def XQ10():
        return Spec("1.0", "XQuery1.0", "XQ", "XQ10")

    @staticmethod
    def XQ30():
        return Spec("3.0", "XQuery3.0", "XQ", "XQ30")

    @staticmethod
    def XQ31():
        return Spec("3.1", "XQuery3.1", "XQ", "XQ31")

    @staticmethod
    def XQ40():
        return Spec("4.0", "XQuery4.0", "XQ", "XQ40")

    @staticmethod
    def XT10():
        return Spec("1.0", "XSLT1.0", "XT", "XT10")

    @staticmethod
    def XT20():
        return Spec("2.0", "XSLT2.0", "XT", "XT20")

    @staticmethod
    def XT30():
        return Spec("3.0", "XSLT3.0", "XT", "XT30")

    @staticmethod
    def XT40():
        return Spec("4.0", "XSLT4.0", "XT", "XT40")

    def process_spec(spec_str: str):
        if spec_str == "XP20":
            return Spec.XP20()
        elif spec_str == "XP30":
            return Spec.XP30()
        elif spec_str == "XP31":
            return Spec.XP31()

        elif spec_str == "XP40":
            return Spec.XP40()

        elif spec_str == "XQ10":
            return Spec.XQ10()

        elif spec_str == "XQ30":
            return Spec.XQ30()

        elif spec_str == "XQ31":
            return Spec.XQ31()

        elif spec_str == "XQ40":
            return Spec.XQ40()

        elif spec_str == "XT30":
            return Spec.XT30()

        elif spec_str == "XT40":
            return Spec.XT40()
        else:
            raise Exception(
                "The specific language must be one of the following: XP20, XP30, XP31, XP40, XQ10, XQ30, XQ31, XQ40, XT30")



class Environment:
    remove_temp_files = []

    @staticmethod
    def environment_cleanup():
        for file in Environment.remove_temp_files:
            if os.path.exists(file):
                if os.getenv("SAXONC_TESTSUITE_DEBUG") is not None:
                    print("Removing file = ", file)
                os.remove(file)
        Environment.remove_temp_files.clear()

    def __init__(self):
        ...
        self.proc = saxonc.PySaxonProcessor(license=True)

        self.need_license = self.proc.edition != "HE"


        self.schema_aware = False

        if(self.need_license):

            try:
                if os.getenv("SAXON_LICENSE_DIR") is not None:
                    self.proc.set_configuration_property("http://saxon.sf.net/feature/licenseFileLocation",
                                                       os.getenv("SAXON_LICENSE_DIR") + "/saxon-license.lic")
            except saxonc.PySaxonApiError as ex:
                self.schema_aware = False
                print("*** unlicensed environment :" + str(ex))
        else:
            self.schema_aware = False

        self.lang = None
        self.proc.set_configuration_property('http://saxon.sf.net/feature/optimizationLevel', optimizer_flags)
        self.proc.set_configuration_property('http://saxon.sf.net/feature/xsd-version', "1.1")

        self.base_uri = ""
        self.spec = None
        '''self.xquery_proc = self.proc.new_xquery_processor()
        self.xpath_proc = self.proc.new_xpath_processor()
        self.xslt_proc = self.proc.new_xslt30_processor()'''
        self.xquery_proc = None
        self.xpath_proc = None
        self.xslt_proc = None
        self.sheet = None
        self.xsd_schema = None

        self.context_item = None

        self.source_docs = {}
        self.param_decimal_declarations = None

        self.param_declarations = None

        self.params = {}

        self.reset_actions = []
        self.usable = True
        self.resources_remove = []

    def create_processors(self):
        self.xpath_proc = self.proc.new_xpath_processor()
        self.xquery_proc = self.proc.new_xquery_processor()
        self.xslt_proc = self.proc.new_xslt30_processor()


def process_environment(cwd_from_base_uri, xpc: saxonc.PyXPathProcessor, env: saxonc.PyXdmNode, spec: Spec, environments_i: dict,
                        environmentDefault: Environment):
    environment = Environment()
    environment.create_processors()
    name = env.get_attribute_value("name")
    if name is not None:
        print("Loading environment " + name)
        environments_i[name] = environment
        '''if name == "works-mod":
            print(env)
            exit(0)'''
    else:
        print("Loading environment without name")
        if os.getenv("SAXONC_TESTSUITE_DEBUG") is not None:
            print(str(env))

    resources = xpc.evaluate("resource")


    if resources is not None:
        resource_xslt_proc = environment.proc.new_xslt30_processor()
        builderi = environment.proc.new_document_builder()
        script_dir = pathlib.Path(__file__).resolve().parent

        base_uri = str(script_dir.as_uri())

        resource_xsl_filename = str(script_dir)+"/XML_Resolver_Catalog_Maker.xsl"
        resources_exec = resource_xslt_proc.compile_stylesheet(stylesheet_file=resource_xsl_filename)
        resource_filename = "testsuite_driver/temp/"+str(name)+".xml"
        env_text = str(env)
        builderi.set_base_uri(env.base_uri)
        env_tempi = builderi.parse_xml(xml_text=env_text)
        resources_exec.set_cwd(cwd_from_base_uri(base_uri))
        resources_exec.apply_templates_returning_file(xdm_value=env_tempi, output_file=resource_filename)
        environment.proc.set_catalog_files([cwd_from_base_uri(base_uri)+"/"+resource_filename])
        Environment.remove_temp_files.append(cwd_from_base_uri(base_uri)+"/"+resource_filename)


    '''environment.create_processors()'''
    xpc.set_context(xdm_item=env)


    '''set the base URI if specified'''
    base_uri = str(env.base_uri)
    print("set_base_uri(base_uri) cp0 = " + base_uri)
    environment.proc.set_cwd(cwd_from_base_uri(base_uri))
    environment.xquery_proc.set_query_base_uri(base_uri)
    '''TODO There is a bug with the DocumentBuilder base_uri and cwd for the processor'''

    base = xpc.evaluate("static-base-uri")
    if base is not None:
        for i in range(base.size):
            basei = base.item_at(i)
            uri = basei.get_node_value().get_attribute_value("uri")
            if uri is None or uri == "UNDEFINED":
                environment.xquery_proc.set_query_base_uri(None)
                environment.xpath_proc.set_base_uri(None)
            else:
                print("base-uri = " + uri)
                if uri != "#UNDEFINED":
                    environment.xquery_proc.set_query_base_uri(uri)
                    environment.xquery_proc.set_cwd(uri)
                    environment.xpath_proc.set_base_uri(uri)
                    environment.xpath_proc.set_cwd(uri)
                    environment.base_uri = uri

    '''set any requested collations - TODO'''

    '''declare the requested namespaces - TODO'''

    decimal_formats = xpc.evaluate("decimal-format")
    if decimal_formats is not None:
        environment.param_decimal_declarations = ""

        for decimal_format in decimal_formats:
            if spec.short_spec_name == "XP":
                '''environment.usable = False
                break'''
            format_name = decimal_format.get_node_value().get_attribute_value("name")
            if format_name is not None:
                '''TODO fixup format_name as EQName'''
                environment.param_decimal_declarations = environment.param_decimal_declarations + "declare decimal-format Q{}" + format_name
            else:
                environment.param_decimal_declarations = environment.param_decimal_declarations + "declare default decimal-format "
            xpc.set_context(xdm_item=decimal_format)
            decimal_format_Atts = xpc.evaluate("@* except @name")

            for decimal_format_Att in decimal_format_Atts:
                propertyi = decimal_format_Att.local_name
                valuei = decimal_format_Att.string_value
                environment.param_decimal_declarations = environment.param_decimal_declarations + " " + (propertyi + "=\"" + valuei + "\" ")
            environment.param_decimal_declarations = environment.param_decimal_declarations + ";"


        xpc.set_context(xdm_item=env)

    '''declare any variable'''
    params = xpc.evaluate("param")

    if params is not None:
        environment.param_declarations = ""


        for parami in params:
            param_node = parami.get_node_value()
            var_name = param_node.get_attribute_value("name")
            source = param_node.get_attribute_value("source")

            parami_value = None

            if source is not None:
                source_doc = environment.source_docs[source]

                if source_doc is None:
                    print("**** Unknown source document " + source)

                parami_value = source_doc
            else:
                select = param_node.get_attribute_value("select")
                parami_value = xpc.evaluate(select)

            statici = param_node.get_attribute_value('static')
            is_static = False
            if statici is not None:
                is_static = True if statici == "true" or statici == "yes" else False
            environment.xpath_proc.declare_variable(var_name)
            declared = False
            declaredi = param_node.get_attribute_value("declared")
            if declaredi is not None:
                declared = True if declaredi == "true" or declaredi == "yes" else False

            if not declared:
                environment.param_declarations = environment.param_declarations + "declare variable $" + var_name + " external; "

            if not is_static:
                environment.params[var_name] = parami_value

            '''else:
                For XSLT we handle this in the xslt30 test suite driver - maybe move logic here
                environment.xslt_proc.set_parameter(var_name, parami_value)
                print("set " + var_name + " = " + str(parami_value))'''







    ns_nodes = xpc.evaluate("namespace")
    if ns_nodes is not None:
        for i in range(ns_nodes.size):
            ns_node = ns_nodes.item_at(i)
            prefix = ns_node.get_node_value().get_attribute_value("prefix")
            uri = ns_node.get_node_value().get_attribute_value("uri")
            environment.xquery_proc.declare_namespace(prefix=prefix, uri=uri)
            environment.xpath_proc.declare_namespace(prefix=prefix, uri=uri)

    '''load the requested schema documents'''

    schema_validator = None

    '''try:
        schema_validator = environment.proc.new_schema_validator()
        schema_validator.set_cwd(cwd_from_base_uri(base_uri))
    except saxonc.PySaxonApiError as ex:
        print("*** Failed to create PySchemaValidator :" + str(ex))'''

    ''' PyXdmNode.base_uri returns file:// - this will cause problems with resolving later.'''
    '''Create bug issue to remove the file:// internally TODO create bug issue '''


    #environment.xsd_schema = load_schema_documents(xpc, environment, builder, xsd_compiler)
    schemas = xpc.evaluate("schema")
    validate_sources = False
    source_arr = []

    xsd_compiler = None

    try:


        if schemas is not None:

            if environment.need_license and environment.proc.edition == "EE":
                xsd_compiler = environment.proc.new_xsd_compiler()
                print("schema found count=" + str(schemas.size))
                for i in range(schemas.size):
                    schema = schemas.item_at(i).get_node_value()
                    role = schema.get_attribute_value("role")
                    xsd_version = schema.get_attribute_value("xsd-version")

                    if xsd_version is not None:
                        # schema_validator.set_property("xsdversion", xsd_version)
                        xsd_compiler.set_xsd_version(xsd_version)
                    else:
                        # schema_validator.set_property("xsdversion", "1.0")
                        xsd_compiler.set_xsd_version("1.0")

                    if role != "secondary":
                        href = schema.get_attribute_value("file")
                        ns = schema.get_attribute_value("uri")

                        if ns == "http://www.w3.org/2005/xpath-functions":
                            # schema_validator.set_property("xsdversion", "1.1")
                            xsd_compiler.set_xsd_version("1.1")

                        if href is None:
                            try:
                                '''schema_validator.register_schema(xsd_file=ns)'''
                                source_arr.append(str(ns))
                            except saxonc.PySaxonApiError as ex:
                                print("*** Failed to load schema by URI: " + ns + " - " + str(ex))


                        else:
                            try:
                                '''schema_validator.register_schema(xsd_file=href)'''
                                full_uri = urljoin(base_uri, href)
                                pathi = pathlib.Path(full_uri.replace("file://", ""))
                                source_arr.append(str(pathi))
                            except saxonc.PySaxonApiError as ex:
                                print("*** Failed to load schema by URI: " + href + " - " + str(ex))

                        xpc.import_schema_namespace(str(ns))

                        environment.xpath_proc.import_schema_namespace(str(ns))

                        if role == "source-reference":
                            validate_sources = True
                        '''if role == "stylesheet-import": '''

                    environment.xsd_schema = xsd_compiler.compile(xsd_list=source_arr)
                    if environment.xsd_schema is not None:
                        environment.xquery_proc.set_schema_aware(True)
                        environment.xpath_proc.use_schema(environment.xsd_schema)
                        environment.xslt_proc.use_schema(environment.xsd_schema)
                        environment.xquery_proc.use_schema(environment.xsd_schema)
            else:
                environment.schema_aware = True
                environment.usable = False
    except saxonc.PySaxonApiError as ex:
        print("*** Failed to load schema by URI: " + str(ex))
        environment.xquery_proc.set_schema_aware(False)
        environment.xpath_proc.set_schema_aware(False)
        environment.xslt_proc.set_schema_aware(False)
        '''environment.usable = False
        return environment'''

    stylesheets = xpc.evaluate("stylesheet[not(@role='secondary')]")
    if stylesheets is not None:
        stylesheet_file = ""
        try:
            for ii in range(stylesheets.size):
                sheet = stylesheets.item_at(ii).get_node_value()
                stylesheet_file = sheet.get_attribute_value("file")
                print(stylesheet_file)
                environment.xslt_proc.set_cwd(cwd_from_base_uri(base_uri))
                environment.sheet = environment.xslt_proc.compile_stylesheet(stylesheet_file=stylesheet_file)
        except saxonc.PySaxonApiError as ex:
            print(ex)
            print("**** failure while compiling environment-defined stylesheet " + stylesheet_file)


    '''environment.xsd_schema =  load_schema_documents(xpc, environment, builder, xsd_compiler)'''
    '''SchemaManager manager = environment.processor.getSchemaManager()
    boolean validateSources = loadSchemaDocuments(driver, xpc, env, environment, manager)'''

    '''load the requested source documents'''

    load_source_documents(xpc, env, environment, environment.xsd_schema, validate_sources)
    return environment

def load_source_documents(xpc: saxonc.PyXPathProcessor, env: saxonc.PyXdmNode, environment: Environment,
                          xsd_schema: saxonc.PyXsdSchema, validate_sources: bool):
    builder = environment.proc.new_document_builder()
    sources = xpc.evaluate("source")
    if sources is not None:
        for i in range(sources.size):

            source = sources.item_at(i).get_node_value()
            raw_uri = source.get_attribute_value("uri")
            role = source.get_attribute_value("role")
            media_type = source.get_attribute_value("media-type")
            validation = source.get_attribute_value("validation")
            if validation is None:
                validation = "skip"
            streaming = source.get_attribute_value("streaming")

            if os.getenv("SAXONC_TESTSUITE_DEBUG") is not None:
                print("Loading source raw_uri = ", raw_uri)

            if not validate_sources and validation == "skip":
                print('validation is skipped')
                '''builder.set_schema_validator(None)
                builder.set_schema_validator(None) 
                TODO this is a bug cannot set validator to None'''
            else:
                try:
                    if xsd_schema is not None:
                        schema_validatori = xsd_schema.new_schema_validator()
                        schema_validatori.set_lax(validation == "lax")
                        builder.set_schema_validator(schema_validatori)
                    '''environment.xpath_proc.use_schema(xsd_schema)'''
                except saxonc.PySaxonApiError as ex:
                    print("*** Failed to create PySchemaValidator :" + str(ex))
            href = source.get_attribute_value("file")
            select = source.get_attribute_value("select")
            xinc = source.get_attribute_value("xinclude")
            content = None
            builder.set_line_numbering(True)

            try:
                if href is not None:
                    base_uri = source.base_uri
                    full_uri = urljoin(base_uri, href)
                    newpath = pathlib.Path(urlparse(full_uri).path)

                    builder.set_base_uri(str(newpath))
                    content = builder.parse_xml(xml_file_name=full_uri)
                    environment.source_docs[href] = content
                    if os.getenv("SAXONC_TESTSUITE_DEBUG") is not None:
                        print("loaded source doc with base_uri = ", base_uri)
                        print("loaded source doc with href = ", href)
                else:
                    '''print(source)'''
                    builder.set_base_uri(source.base_uri)
                    xpc.set_context(xdm_item=source)
                    contenti = xpc.evaluate_single("string(content)")
                    content = builder.parse_xml(xml_text=contenti.string_value)

                    if href is None:
                        environment.source_docs["default"] = content
                    else:
                        environment.source_docs[href] = content
                if role is not None:
                    if "." == role:
                        if href is None:
                            environment.context_item = environment.source_docs["default"]
                        else:
                            environment.context_item = environment.source_docs[href]

                    elif role.startswith("$"):
                        var_name = role[1:]

                        environment.params[var_name] =content
                        environment.xpath_proc.declare_variable(var_name)

                        declared = source.get_attribute_value("declared")
                        is_declared = (
                                declared is not None and
                                declared.lower() in ("true", "yes", "1")
                        )

                        if not is_declared:
                            if environment.param_declarations is None:
                                environment.param_declarations = ""

                            environment.param_declarations += (
                                f"declare variable ${var_name} external; "
                            )

            except saxonc.PySaxonApiError as ex:
                print("Failed loading source raw_uri = ", raw_uri)
                print("*** failed to build source document " + str(ex))