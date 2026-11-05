import pytest
import saxon_import
saxonc = saxon_import.import_star(globals())

class Test30Processor:
    saxon_processor = PySaxonProcessor()

    @pytest.fixture
    def processor(self):
        return self.saxon_processor.new_xpath_processor()

    def test_base_uri_is_empty_string_by_default(self, processor):
        assert processor.base_uri == ""

    def test_base_uri_can_be_set(self, processor):
        processor.set_base_uri("http://example.org/")
        assert processor.base_uri == "http://example.org/"

    @pytest.mark.skip('Skipping because of https://saxonica.plan.io/issues/7266')
    def test_base_uri_can_be_unset(self, processor):
        processor.set_base_uri("http://example.org/")
        processor.set_base_uri("")
        assert processor.base_uri == ""
