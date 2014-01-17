exports.dataDAO = function(mysql) {

  this.getLastsDatasByMinute = function(minutes, callBack) {  
    var queryText = "select * from (select *,str_to_date(Date,'%a %b %e %H:%i:%s %Y') "
      + "as Time from temperatures) as t INNER JOIN nodesOnline ON t.nodeIP="
      + "nodesOnline.nodeID where t.Time > NOW() - INTERVAL "+minutes+" MINUTE ORDER BY Time DESC;";
    mysql.query(queryText,
        function(err, result, fields) {
          if (err) throw err;
          else {
            callBack(result);
          }
        });
  }  

  this.getAllNodes = function(callBack) {  
    var queryText = "select distinct Prefix from temperatures;"
    mysql.query(queryText,
     function(err, result, fields) {
      if (err) throw err;
      else {
        callBack(result);
      }
     });
  }  

  this.getAllFrom = function(nodeName, callBack) {
    var queryText = "select * from temperatures where Prefix = '"+nodeName+"'";
    mysql.query(queryText,
      function(err, result, fields) {
        if (err) throw err;
        else {
          callBack(result);
        }
      });
  }
}