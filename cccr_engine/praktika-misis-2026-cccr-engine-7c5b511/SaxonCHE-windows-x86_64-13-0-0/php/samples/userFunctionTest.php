<!DOCTYPE html SYSTEM "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <title>SaxonC API design use cases</title>
</head>
<body>
<?php

function userFunction($param2)
{

  echo("user function called\n");
  if(is_numeric($param2)){
    $result = $param2*2;
    return $result;
  }

  return null;

}


function sumASequence($arr){
  echo("sumASequence called\n");
  if(is_array($arr)){
    return array_sum($arr);
  }
  echo(gettype($arr));
  echo("Error in sumASequence\n");
  throw new Exception('$arr argument is not an array type');

}

function simpleUserFunction2($input){
  echo("simpleUserFunction2 called\n");
  return $input;

}

function twoSequenceParam2($arr1, $arr2){
  echo("twoSequenceParam2 called\n");

    return $arr2;
}

function implode_all($arr) {
    for ($i=0; $i<count($arr); $i++) {
        if (@is_array($arr[$i]))
            $arr[$i] = implode_all ($arr[$i]);
    }
    return implode(' ', $arr);
}

function twoSequenceParam3($arr1, $arr2){
  echo("twoSequenceParam3 called\n");
  if(is_array($arr1) and is_array($arr2)){
    $out1 = implode_all($arr1);
    $out2 = implode_all($arr2);
    echo $out1."\n";
    echo $out2."\n";
    return $arr2;
  } else {
    echo("twoSequenceParam3 array not found\n");
    return NULL;
  }
}

function twoSequenceParam($arrIter1, $arrIter2){
  echo("twoSequenceParam called\n");
  if($arrIter1 != NULL and $arrIter2 != NULL){

    while( $arrIter1->valid() ){
      echo $arrIter1->key() . "=" . $arrIter1->current() . "\n";
      $arrIter1->next();
    }

    while( $arrIter2->valid() ){
      echo $arrIter2->key() . "=" . $arrIter2->current() . "\n";
      $arrIter2->next();
    }

    return True;
  } else {
    throw new Exception('$arr argument is not an array type');
  }
    return False;
}

function sequenceOfArraysParam($arr1, $arr2){
  echo("sequenceOfArraysParam called\n");
  if(is_array($arr1) and is_array($arr2)){
    $out1 = implode_all($arr1);
    $out2 = implode_all($arr2);
    echo $out1."\n";
    echo $out2."\n";
    return True;
  } else {
    throw new Exception('$arr argument is not an array type');
  }
    return False;

}

function sequenceOfArraysParamB($arrIter){
  echo("sequenceOfArraysParamB called\n");
  if($arrIter != NULL){

    while( $arrIter->valid() ){
        $arri = $arrIter->current();
        if(is_array($arri)){
            $out1 = implode_all($arri);
            echo $out1."\n";
        }
        $arrIter->next();
    }
   return True;
  } else {
    throw new Exception('$arr argument is not an array type');
  }
    return False;
}


function sequenceOfArraysParamC($arr1){
  echo("sequenceOfArraysParamC called\n");
  if(is_array($arr1)){
    $out1 = implode_all($arr1);
    echo $out1."\n";
    return new ArrayIterator(array($arr1));
  } else {
    throw new Exception('$arr argument is not an array type');
  }
    return $arr1;
}

function sequenceOfMapParam($map1, $map2){
  echo("sequenceOfMapParam called\n");
  if(is_array($map1)){
    $out1 = implode_all($map1);
    echo $out1."\n";
    return True;
  } else {
    throw new Exception('$map argument is not an array type');
  }
    return False;
}

function attributeParam($input){
  echo("attributeParam called\n");
  return $input != NULL && ($input instanceof Saxon\XdmNode) && $input->getNodeKind() == 2;
}


function documentParam($input){
  echo("documentParam called\n");
  return $input != NULL && ($input instanceof Saxon\XdmNode) && $input->getNodeKind() == 9;
}

function elementParam($input){
  echo("elementParam called\n");
  return $input != NULL && ($input instanceof Saxon\XdmNode) && $input->getNodeKind() == 1;
}

function nodeParam($input){
  echo("nodeParam called\n");
  return $input != NULL && ($input instanceof Saxon\XdmNode) && $input->getNodeKind() >= 1 && $input->getNodeKind() <= 13;
}

function nodeParam2($input){
  echo("nodeParam called\n");
  if(($input instanceof Saxon\XdmNode) && $input->getNodeKind() >= 1 && $input->getNodeKind() <= 13){
    echo("nodeParam child\n");
    $child = $input->getChildNode(0);
    if($child == null) {
           echo("nodeParam child is null\n");
    }
    return $child;
  }else {
        return null;
  }
}


function testSequence($saxonProc) {
         $saxonProc->registerUserFunction("http://www.example.com", "{ \"sumASequence\": {".
                                                        "      \"as\": \"xs:integer?\"," .
                                                        "      \"param\": [" .
                                                        "        \"array(xs:integer*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"simpleUserFunction2\": {".
                                                        "      \"as\": \"xs:integer?\"," .
                                                        "      \"param\": [" .
                                                        "        \"xs:integer\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", False);

         $xpath = $saxonProc->newXPathProcessor();


         $xpath->declareNamespace("dy", "http://www.example.com");

         try {
             $result = $xpath->evaluateSingle("dy:sumASequence([1,2,3,4,5])");
             if($result != null) {
                $value = $result->getAtomicValue()->getLongValue();
                echo 'Correct Result='.$value;
                echo "\n";
             }

             $result2 = $xpath->evaluateSingle("dy:simpleUserFunction2(1)");
             if($result2 != null) {
                $value = $result2->getAtomicValue()->getLongValue();
                echo 'Correct Result2='.$value;
                echo "\n";
             }
         } catch (Exception $e) {
             echo 'Caught exception: ', $e->getMessage(), "\n";
         }

}

function testSimpleUserFunction($saxonProc1){
      $saxonProc1->registerUserFunction("http://www.example.com", "{ \"userFunction\": {".
                                                            "      \"as\": \"xs:integer?\"," .
                                                            "      \"param\": [" .
                                                            "        \"xs:integer\"" .
                                                            "      ],".
                                                            "      \"arity\": [1]" .
                                                            "    }".
                                                            "}", True);



     $xpath = $saxonProc1->newXPathProcessor();
     $xpath->declareNamespace("dy", "http://www.example.com");

     try {
         $result = $xpath->evaluateSingle("dy:userFunction(3)");
         if($result != null) {
            $value = $result->getAtomicValue()->getLongValue();
            echo 'Result='.$value;
            echo "\n";
         }
     } catch (Exception $e) {
         echo 'Caught exception: ', $e->getMessage(), "\n";
     }

}


function testTwoSequence($saxonProc) {
         $saxonProc->registerUserFunction("http://www.example.com", "{ \"twoSequenceParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"xs:string*\", \"xs:integer*\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"twoSequenceParam2\": {".
                                                        "      \"as\": \"xs:integer*\"," .
                                                        "      \"param\": [" .
                                                        "        \"xs:string*\", \"xs:integer*\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", False);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"twoSequenceParam3\": {".
                                                        "      \"as\": \"array(xs:integer*)\"," .
                                                        "      \"param\": [" .
                                                        "        \"array(*)\", \"array(*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $xpath = $saxonProc->newXPathProcessor();


         $xpath->declareNamespace("ms", "http://www.example.com");

         try {
             $result = $xpath->evaluateSingle("ms:twoSequenceParam(('a','b','c'), (1,2,3))");
             if($result != null) {
                $value = $result->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
                echo "\n";
             }

             $result2 = $xpath->evaluate("ms:twoSequenceParam2(('a','b','c'), (1,2,3))");
             if($result2 != null) {
                $size = $result2->size();
                $value = $result2->itemAt(0)->getStringValue();
                echo 'Output size='.$size." Result for 1st item = ".$value;
                echo "\n";
             }
             $result3 = $xpath->evaluate("ms:twoSequenceParam3(['a','b','c'], [1,2,3])");
             if($result3 != null) {
                $size = $result3->size();
                $value = $result3->itemAt(0);
                if($value->isArray()) {
                    $arrValue1 = $value->getArray();
                    echo 'Output size='.$size." Result for 1st item = ".$arrValue1->get(0);
                    echo "\n";
                }
             }
         } catch (Exception $e) {
             echo 'Caught exception: ', $e->getMessage(), "\n";
         }

}

function commonSamplesForSequences($saxonProc) {
         $saxonProc->registerUserFunction("http://www.example.com", "{ \"sequenceOfArraysParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"array(*)\", \"array(*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"sequenceOfArraysParamB\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"array(*)*\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"sequenceOfArraysParamC\": {".
                                                        "      \"as\": \"array(xs:integer*)*\"," .
                                                        "      \"param\": [" .
                                                        "        \"array(*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"sequenceOfMapParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"map(*)\", \"map(*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $xpath = $saxonProc->newXPathProcessor();


         $xpath->declareNamespace("ms", "http://www.example.com");

         try {
             $result = $xpath->evaluateSingle("ms:sequenceOfArraysParam(array{1,2,3}, array{'a',2,3})");
             if($result != null) {
                $value = $result->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
                echo "\n";
             }

             $result1 = $xpath->evaluateSingle("ms:sequenceOfArraysParamB((array{1,2,3}, array{'a',2,3}))");
             if($result1 != null) {
                $value = $result1->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
                echo "\n";
             }

             $result1a = $xpath->evaluate("ms:sequenceOfArraysParamC(array{1,2,3})");
             if($result1a != null) {
                if($result1a instanceof Saxon\XdmValue) {
                    echo("Test sequenceOfArraysParamC: XdmValue found\n");
                    $value1 = $result1a->getHead();
                    if($value1->isArray()) {
                        $arrValue1 = $value1->getArray();
                        if($arrValue1 instanceof Saxon\XdmArray) {
                            echo 'Test sequenceOfArraysParamC: XdmArray found='.$value1;
                            echo "\n";
                        } else {
                            echo("sequenceOfArraysParamC failed type check\n");
                        }
                    } else {
                        echo("sequenceOfArraysParamC failed type check\n");
                    }

                }
                if($result1a instanceof Saxon\XdmArray) {
                    echo 'Error: Test sequenceOfArraysParamC: XdmArray found';
                    echo "\n";
                }
            }

             $result2 = $xpath->evaluateSingle("ms:sequenceOfMapParam(map{'a':1,'b':2},map{'c':3,'d':'four'})");
             if($result1 != null) {
                $value = $result1->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
             }
         } catch (Exception $e) {
             echo 'Caught exception: ', $e->getMessage(), "\n";
         }

}

function handlingXdmNodes($saxonProc) {
         $saxonProc->registerUserFunction("http://www.example.com", "{ \"elementParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"element(*)\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com",  "{ \"documentParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"document-node()\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", False);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"nodeParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"node()\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", False);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"attributeParam\": {".
                                                        "      \"as\": \"xs:boolean\"," .
                                                        "      \"param\": [" .
                                                        "        \"attribute()\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", True);

         $saxonProc->registerUserFunction("http://www.example.com", "{ \"nodeParam2\": {".
                                                        "      \"as\": \"node()\"," .
                                                        "      \"param\": [" .
                                                        "        \"node()\"" .
                                                        "      ],".
                                                        "      \"arity\": [1]" .
                                                        "    }".
                                                        "}", False);

         $xpath = $saxonProc->newXPathProcessor();


         $xpath->declareNamespace("ms", "http://www.example.com");

         try {
             $result = $xpath->evaluateSingle("ms:elementParam(parse-xml('<doc/>')/*)");
             if($result != null) {
                $value = $result->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
             }

             $result1 = $xpath->evaluateSingle("ms:documentParam(parse-xml('<doc/>'))");
             if($result1 != null) {
                $value = $result1->getAtomicValue()->getBooleanValue();
                if($value){
                echo 'documentParam passed';
                } else {
                echo 'documentParam failed';
                }
             }

             $result2 = $xpath->evaluateSingle("ms:nodeParam(parse-xml('<doc/>')/node())");
             if($result2 != null) {
                $value = $result2->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
             }

             $result3 = $xpath->evaluateSingle("ms:attributeParam(parse-xml('<doc a=\"1\"/>')/*/@a)");
             if($result3 != null) {
                $value = $result3->getAtomicValue()->getBooleanValue();
                echo 'Correct Result='.$value;
             }

             $result4 = $xpath->evaluateSingle("ms:nodeParam2(parse-xml('<doc><elem>text</elem></doc>')/node())");
             if($result4 != null && $result4->isNode()) {
                $value = $result4->getNodeValue();
                $strVal = $value->getStringValue();
                if($strVal != NULL) {
                    echo 'Correct Result='.$strVal;
                } else {
                   echo 'nodeParam2 failed';
                }


             }
         } catch (Exception $e) {
             echo 'Caught exception: ', $e->getMessage(), "\n";
         }

}



$saxonProc1 = new Saxon\SaxonProcessor(true);
$xpath = $saxonProc1->newXPathProcessor();

$version = $saxonProc1->version();


echo '<b>PHP User Function Example</b><br/>';
echo 'Saxon Processor version: ' . $version;
echo '<br/>';




testSimpleUserFunction($saxonProc1);

testSequence($saxonProc1);

testTwoSequence($saxonProc1);

commonSamplesForSequences($saxonProc1);

handlingXdmNodes($saxonProc1);


?>
</body>
</html>
