# TP1 - pf le cascadeur

L'objectif du TP est de développer un utilitaire `pf` qui mesure les ressources utilisés par une commande.

## Usage

`pf [-u|-c|-a] [-n n] [-s] commande [argument...]`

L’utilitaire exécute une commande, éventuellement plusieurs fois, et affiche le temps réel et/ou utilisateur consommé par la commande et/ou le nombre d'instructions machines utilisés par la commande.

C'est une version simplifiée des utilitaires `time` et `perf`.

###

Par défaut, `pf` présente le nombre de secondes écoulées en temps réel par la commande (deux chiffre de précision après le point).
L'affichage se fait sur la sortie standard d'erreur.

```
$ ./pf ./work
1.00
```

Ceci est analogue à ce que retourne la commande `time` avec `%e`.

```
$ command time -f "%e" ./work
1.00
```

Bien sur, la commande peut être n'importe quel programme valide et ses arguments.

```
$ ./pf sleep 1.5
1.50
```

Attention, les options de `pf` apparaissent nécessairement avant la commande. Si une option apparait après la commande elle considérée comme un argument de la commande.

### -u

Avec l'option `-u`, `pf` présente le nombre de secondes écoulées en temps utilisateur par la commande (deux chiffre de précision après le point)

```
$ ./pf -u ./work
0.66
$ ./pf -u sleep 1
0.00
```

Ceci est analogue à ce que retourne la commande `time` avec `%u`.

```
$ command time -f "%U" ./work
0.66
$ command time -f "%U" sleep 1
0.00
```

### -c

Avec l'option `-c`, `pf` présenter le nombre de cycles processeur consommés par la commande en mode utilisateur.

```
$ ./pf -c ./work
140938200
```

Ceci est analogue au compteur `cycles` de `perf stat`. C'est le premier nombre affiché par la commande de l'exemple suivant.

```
$ perf stat -e cycles -x, ./work
142304586,,cycles:u,1000201099,100,00,,
```


Pour les détails, man `perf stat`. `perf` est un outil puissant offert par le noyau Linux.
Je vous invite à regarder ce qu'il est possible de faire avec.

### -a

Avec l'option `-a`, l'utilitaire `pf` affiche les trois valeurs : le temps réel, un espace, le temps utilisateur, un espace, et le nombre de cycles processeurs.

```
$ ./pf -a ./work
1.00 0.61 143603651
$ ./pf -a sleep 1.1
1.10 0.00 1211159
```


### -n

L'option `-n` indique un nombre de répétition.
La commande est ainsi exécutée autant de fois qu'indiqué avec un affichage des valeurs de chaque exécution.

Si n est supérieur à 1, une ligne supplémentaire est affichée en dernier qui contient les valeurs moyennes de l'ensemble des exécutions.

```
./pf -n 5 ./work
1.00
1.00
0.99
1.00
1.00
1.00
```

Les commandes sont exécutées l'une à la suite de l'autre.
Ainsi l'exemple précédent a pris 5 secondes de temps réel pour s'exécuter.

On pourrait utiliser `time` pour mesurer le temps réel disponible mais `./pf` fait aussi l'affaire.
Par exemple, un premier `pf` exécute et mesure un second `pf` qui exécute deux `sleep 1` :

```
./pf ./pf -n 2 sleep 1
1.00
1.00
1.00
2.00
```



Bien évidemment, `-n` est compatible avec les autres options de l'outil.

```
./pf -a -n 3 sleep 1.1
1.10 0.00 454595
1.10 0.00 441480
1.10 0.00 430592
1.10 0.00 442222
```

### -s

L'option `-s` exécute la commande dans un sous shell `/bin/sh` via l'option `-c`.
Cela permet de mesurer des commandes complexes.
Bien sur, le cout du shell est mesuré aussi.

```
./pf -s './work; sleep .5'
1.50
```

Bien évidemment, `-s` est compatible avec les autres options de l'outil.

```
./pf -a -n 3 -s './work& ./work; wait'
1.00 1.32 278797959
1.00 1.34 278431654
1.00 1.33 275326470
1.00 1.33 277518694
```



### Valeur de retour

La valeur de retour est celle de la dernière commande qui a fini de s'exécuter.

S'il y a un problème avec l'exécution de la commande, `pf` affiche un message d'erreur et termine avec une valeur de retour de 127.


## Contraintes d'implémentation

Le parallélisme se fera avec des fork et non avec des threads.

Pour mesurer le temps réel, vous utiliserez l'appel système POSIX `clock_gettime` avec l'horloge `CLOCK_MONOTONIC`.

Pour déterminer le temps utilisateur consommé, vous utiliserez l'appel système Linux `wait4`.

Pour déterminer le nombre de cycles CPU utilises, vous utiliserez l'appel système Linux `perf_event_open` et le compteur `PERF_COUNT_HW_CPU_CYCLES`.

Attention, pour fonctionner, il est important d'avoir un Linux pas trop ancien (à partir de 2.6.32, décembre 2009) et que le noyau soit compilé et configuré pour permettre l'utilisation de `perf_event_open` ce qui est le cas de la plupart des distributions Linux actuelles.
Vérifiez que `/proc/sys/kernel/perf_event_paranoid` existe et a une valeur inférieure ou égale à 2. Dans le cadre du TP, une valeur 2 (qui est celle par défaut) est suffisante, une valeur plus faible permet d’accéder à des informations plus bas niveau non nécessaire pour le TP mais toujours intéressante à explorer.


Le projet a été développé sur Gitlab.
