#!/bin/bash
if [[ $# -ne 2 ]]; then
  echo "usa: $(basename $0) <pid.txt> <logFilename.txt>"
  exit -1
fi

if [[ ! -f $1 || ! -f $2 ]]; then
  echo "i due argomenti devono essere file di testo"
  exit -1
fi

pid=`cat $1`

while ps -p $pid > /dev/null; do
  sleep 1;
done

log=`cat $2`

exec 3<$log

printf "\t\t\tSTATISTICHE DEL SUPERMERCATO\n"
cliente=0
cassa=0
total_apertura=0
sum_servizio=0

while read -r -u 3 linea; do

  token=($linea)

  x=$(echo $linea | grep 'SUPERMERCATO'| wc -l);
  if [ $x -eq 1 ]; then
    echo Numero di clienti serviti = ${token[1]}
    echo Numero di prodotti acquistati = ${token[2]}
    continue
  fi

  x=$(echo $linea | grep 'CLIENTE'| wc -l);
  if [ $x -eq 1 ]; then
    if [ $cliente -eq 0 ]; then
      echo "| id cliente | n. prodotti acquistati | tempo totale nel super. | tempo tot. speso in coda | n. di code visitate |"
      cliente=1
    fi
    if (( $(echo "${token[3]} != 0" | bc -q) )); then
      (( token[5] += 1 ))
    fi
    printf "| %4s | %4s | %7s | %7s | %2s |\n" ${token[1]} ${token[6]} ${token[2]} ${token[3]} ${token[5]}
    continue
  fi

  x=$(echo $linea | grep 'CASSA'| wc -l);
  if [ $x -eq 1 ]; then
    if [ $cassa -eq 0 ]; then
      echo "| id cassa | n. prodotti elaborati | n. di clienti | tempo tot. di apertura | tempo medio di servizio | n. di chiusure |"
      cassa=1
    fi
    id_cassa=${token[1]}
    prod_elaborati=${token[3]}
    num_clienti=${token[2]}
    num_chiusure=${token[4]}
    continue
  fi

  x=$(echo $linea | grep 'APERTURA'| wc -l);
  if [ $x -eq 1 ]; then
    total_apertura=$(echo "scale=3; $total_apertura + ${token[1]}"| bc -q )
    continue
  fi

  x=$(echo $linea | grep 'SERVIZIO'| wc -l);
  if [ $x -eq 1 ]; then
    sum_servizio=$(echo "scale=3; $sum_servizio + ${token[1]}" | bc -q)
    continue
  fi

  x=$(echo $linea | grep -E 'FINE'| wc -l);
  if [ $x -eq 1 ]; then
    if [ $num_clienti -ne 0 ]; then
      servizio_medio=$(echo "scale=3; $sum_servizio/$num_clienti"| bc -q )
    else
      servizio_medio=0
    fi

    printf "| %4s | %6d | %5d | %7s | %7s | %4d |\n" $id_cassa $prod_elaborati $num_clienti $total_apertura $servizio_medio $num_chiusure
    total_apertura=0
    sum_servizio=0
    continue

  fi


done


exit 0
