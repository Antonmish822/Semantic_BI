import pytest
import saxon_import

saxonc = saxon_import.import_star(globals())
from pathlib import Path
import os

@pytest.fixture
def data_dir(pytestconfig):
    return pytestconfig.getoption("--data-dir")

@pytest.fixture
def saxonproc():
    return PySaxonProcessor()

class TestXQueryProcessor:
    saxon_processor = PySaxonProcessor()

    @pytest.fixture
    def processor(self):
        return self.saxon_processor.new_xquery_processor()

    def test_static_base_uri_defaults_to_cwd(self, processor):
        expected = "%s/" % Path.cwd().as_uri()
        processor.set_query_content("static-base-uri()")
        actual = str(processor.run_query_to_value())
        assert actual == expected

    def test_set_static_base_uri(self, processor, data_dir):
        expected = Path(data_dir).joinpath("trax/xml").as_uri()
        processor.set_query_base_uri(expected)
        processor.set_query_content("static-base-uri()")
        actual = str(processor.run_query_to_value())
        assert actual == expected

    def test_collection_resolves_against_base_uri(self, processor, data_dir):
        base_folder_uri = "%s/" % Path(data_dir).joinpath("trax/xml").as_uri()
        processor.set_query_base_uri(base_folder_uri)
        processor.set_query_content(f"collection('?select=*.xml')")
        r = processor.run_query_to_value()
        assert r is not None


def test_return_document_node(saxonproc):
    node = saxonproc.parse_xml(xml_text='<foo/>')
    assert node is not None
    xqc = saxonproc.new_xquery_processor()
    xqc.set_query_content('document{.}')
    xqc.set_context(xdm_item=node)
    result = xqc.run_query_to_value()
    if isinstance(result, PyXdmNode):
        assert result.node_kind == DOCUMENT


def testxQuery1(tmp_path, saxonproc, data_dir):
    query_proc = saxonproc.new_xquery_processor()
    query_proc.set_cwd(str(tmp_path))
    query_proc.clear_properties()
    query_proc.clear_parameters()
    xmlFile = os.path.join(data_dir, "cat.xml")
    query_proc.set_property("s", xmlFile)
    query_proc.set_property("qs", "<out>{count(/out/person)}</out>")
    result = query_proc.run_query_to_string()
    assert result is not None
    print(result)

    cat_output_filename = "catOutput.xml"
    cat_output_path = tmp_path / cat_output_filename
    query_proc.run_query_to_file(output_file_name = cat_output_filename)
    assert cat_output_path.is_file()
    node = saxonproc.parse_xml(xml_file_name = str(cat_output_path))
    assert node is not None
    xp = saxonproc.new_xpath_processor()
    xp.set_context(xdm_item=node)
    assert xp.effective_boolean_value("/out/text()=3")

def testxQuery2(tmp_path, saxonproc, data_dir):
    query_proc = saxonproc.new_xquery_processor()
    query_proc.set_cwd(str(tmp_path))
    saxonproc.set_cwd(str(tmp_path))
    query_proc.clear_properties()
    query_proc.clear_parameters()
    xmlFile = os.path.join(data_dir, "cat.xml")
    query_proc.set_property("s", xmlFile)
    query_proc.set_property("qs", "<out>{count(/out/person)}</out>")
    result = query_proc.run_query_to_string()
    assert result is not None
    print(result)

    cat_output_filename = "catOutput2.xml"
    cat_output_path = tmp_path / cat_output_filename
    query_proc.run_query_to_file(output_file_name = cat_output_filename)
    assert cat_output_path.is_file()
    builder = saxonproc.new_document_builder()

    node = builder.parse_xml(xml_file_name = str(cat_output_path))
    assert node is not None
    xp = saxonproc.new_xpath_processor()
    xp.set_context(xdm_item=node)
    assert xp.effective_boolean_value("/out/text()=3")


def testxQuery_encoding1(saxonproc, data_dir):
    xml = """\
    <out>
        <person att1='value1' att2='value2'>text1</person>
        <salary>3000 €</salary>
        <person>text2</person>
        <person>text3</person>
    </out>
    """
    query_proc = saxonproc.new_xquery_processor()
    query_proc.set_cwd(os.getcwd() + '/')
    node = saxonproc.parse_xml(xml_text=xml, encoding="UTF-8")
    result = query_proc.run_query_to_value(input_xdm_item=node, query_text="/out/salary/text() = '3000 €'",
                                           encoding="UTF-8")
    assert result.head.get_atomic_value().boolean_value
    print(result)


def test_default_namespace(saxonproc):
    query_proc = saxonproc.new_xquery_processor()
    query_proc.declare_namespace("", "http://one.uri/")
    node = saxonproc.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar/></foo>")
    query_proc.set_context(xdm_item=node)
    query_proc.set_query_content("/foo")
    value = query_proc.run_query_to_value()
    assert value.size == 1


def test_document_builder(saxonproc):
    builder = saxonproc.new_document_builder()
    builder.set_base_uri("file:/tmp")
    node = builder.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar/></foo>")
    assert node is not None
    assert node.base_uri == "file:/tmp"


def test_static_base_uri(saxonproc):
    xquery1 = '"Static base uri : " || static-base-uri()'

    xqueryProcessor = saxonproc.new_xquery_processor()
    workingDirPath = str(Path.cwd().absolute())

    assert workingDirPath is not None

    xqueryProcessor.set_cwd(workingDirPath)

    try:
        result = xqueryProcessor.run_query_to_string(query_text=xquery1)
        assert result is not None
    except  Exception as err:
        assert False


def test_static_base_http_uri(saxonproc):
    xquery1 = '"Static base uri : " || static-base-uri()'

    xqueryProcessor = saxonproc.new_xquery_processor()
    workingDirPath = 'http://www.example.com/base'

    assert workingDirPath is not None

    xqueryProcessor.set_cwd(workingDirPath)

    try:
        result = xqueryProcessor.run_query_to_string(query_text=xquery1)
        assert result is not None
        assert 'http://' in result
    except  Exception as err:
        assert False


def test_default_namespace2(saxonproc):
    query_proc = saxonproc.new_xquery_processor()
    query_proc.declare_namespace("", "http://one.uri/")
    node = saxonproc.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar/></foo>")
    value = query_proc.run_query_to_value(input_xdm_item=node, query_text="/foo")
    assert value.size == 1


def test_XQuery_line_number():
    ''' No license file given therefore result will return None'''
    try:
        proc = PySaxonProcessor(license=True)
        proc.set_configuration_property("l", "on")
        query_proc = proc.new_xquery_processor()

        query_proc.set_property("s", "cat.xml")
        query_proc.declare_namespace("saxon", "http://saxon.sf.net/")
        query_proc.set_property("qs", "saxon:line-number(doc('cat.xml')/out/person[1])")
        result = query_proc.run_query_to_string()
        assert result == None
    except  Exception as err:
        print("Error: ", err)


def testReusability(saxonproc):
    queryproc = saxonproc.new_xquery_processor()
    queryproc.clear_properties()
    queryproc.clear_parameters()
    input_ = saxonproc.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar xmlns='http://two.uri'>12</bar></foo>")
    queryproc.declare_namespace("", "http://one.uri/")
    queryproc.set_query_content("declare variable $p as xs:boolean external; exists(/foo) = $p")
    queryproc.set_context(xdm_item=input_)
    value1 = saxonproc.make_boolean_value(True)
    queryproc.set_parameter("p", value1)
    result = queryproc.run_query_to_value()
    item = result.head
    assert result is not None
    assert item.is_atomic
    assert item.boolean_value
    queryproc.clear_parameters()
    queryproc.clear_properties()

    queryproc.declare_namespace("", "http://two.uri")
    queryproc.set_query_content("declare variable $p as xs:integer external; /*/bar + $p")

    queryproc.set_context(xdm_item=input_)
    value2 = saxonproc.make_long_value(6)
    queryproc.set_parameter("p", value2)

    result2 = queryproc.run_query_to_value()
    item2 = result2.head
    assert item2.integer_value == 18


def testQueryKeyWords(saxonproc, data_dir):
    queryproc = saxonproc.new_xquery_processor()
    queryproc.clear_properties()
    queryproc.clear_parameters()
    input_ = saxonproc.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar xmlns='http://two.uri'>12</bar></foo>")
    queryproc.declare_namespace("", "http://one.uri/")
    value1 = saxonproc.make_boolean_value(True)
    queryproc.set_parameter("p", value1)
    foo_file = os.path.join(data_dir, "foo.xq")
    result = queryproc.run_query_to_value(query_file=foo_file, input_xdm_item=input_)
    item = result.head
    assert result is not None
    assert item.is_atomic
    assert item.boolean_value
    queryproc.clear_parameters()
    queryproc.clear_properties()


def testQueryKeyWords_string(saxonproc, data_dir):
    queryproc = saxonproc.new_xquery_processor()
    queryproc.clear_properties()
    queryproc.clear_parameters()
    input_ = saxonproc.parse_xml(xml_text="<foo xmlns='http://one.uri/'><bar xmlns='http://two.uri'>12</bar></foo>")
    queryproc.declare_namespace("", "http://one.uri/")
    value1 = saxonproc.make_boolean_value(True)
    queryproc.set_parameter("p", value1)
    foo_file = os.path.join(data_dir, "foo.xq")
    result = queryproc.run_query_to_string(query_file=foo_file, input_xdm_item=input_)
    assert result is not None
    queryproc.clear_parameters()
    queryproc.clear_properties()


def test_make_string_value(saxonproc):
    xdm_string_value = saxonproc.make_string_value('text1')

    print(xdm_string_value)
    xquery_processor = saxonproc.new_xquery_processor()
    xquery_processor.set_parameter('s1', xdm_string_value)
    result = xquery_processor.run_query_to_value(query_text='declare variable $s1 external; $s1')
    item1 = result.head

    assert result is not None
    assert isinstance(item1, PyXdmAtomicValue)
    assert item1.string_value == "text1"
