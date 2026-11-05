xquery version "3.0";
declare namespace fots = "http://www.w3.org/2012/08/qt-fots-results";
declare variable $results-root as xs:anyURI external;
<results>
{
  for $test in collection(concat($results-root, "?recurse=yes"))//fots:test-case
  let $res := $test/@result
  group by $res
  order by $res
  return <status count="{count($test)}" status="{$res}"/>
}
</results>
