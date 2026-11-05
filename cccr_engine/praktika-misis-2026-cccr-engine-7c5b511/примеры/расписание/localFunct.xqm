(:~
 : Функции для TRaC.
 :
 : @author Александр Калинин и Сергей Мишуров, Artel 2019-2023, BSD License
 :)

module namespace localFunct = 'localFunct';


(:~
 : Возвращает результаты вызова компонента
 : @param  $component имя компонента с запросом SPARQL
 : @return xs:string*
 :)
declare 
  %public
function 
localFunct:component($component as xs:string)
  as element(_)*
{
  let $rdf_endpoint := "https://rdf.lipers24.ru/lipers24.ru"
  let $rq_path := replace("/srv/nextcloud/data/lipers24.ru/files/lipers24.ru/компоненты/%1/данные.rq", "%1",$component)
  let $data as element(_)* := 
    localFunct:query(
      fetch:text($rq_path),
      $rdf_endpoint
    )//bindings/_
  return
    $data
};


(:~
 : Возвращает идентификатор, если найден. Если сущность не найдена, возвращает пустое множество
 : @param  $context контекст текущей точки обработки схемы
 : @param  $component имя компонента с запросом SPARQL
 : @return xs:string*
 :)
declare 
  %public
function 
localFunct:about($context as element(context), $component as xs:string)
  as xs:string*
{
  let $data as element(_)* := localFunct:component($component)
  let $about := $data[*:id[*:value/text() = $context/id/text()]][1]/*:about/*:value/text()
  return
    if($about)then($about)else($context/id.alt/text())
};

declare
  %public
function localFunct:query(
  $sparql as xs:string,
  $endPoint as xs:string
) as item()*
{
  let $response :=
    try{
      fetch:text(
        web:create-url(
          $endPoint || '/sparql',
          map{
            'query':$sparql,
            'output':'json'
          }
        )
      )
    }catch*{
      false()
    }
  return
    if($response)
    then(
      try{
        json:parse($response)
      }catch*{
        <err:parse>Ошибка парсинга</err:parse>
      }
    )
    else(
      <err:request>Ошибка запроса</err:request>
    ) 
};

