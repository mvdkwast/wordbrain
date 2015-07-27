wordbrain trouve les solutions du jeu Wordbrain de MagInteractive.

Compilation
===========

Ce programme nécessite la librairie Qt, version >= 4.0 .

Compiler avec 'qmake', suivi de 'make', vous devez obtenir un exécutable nommé
wordbrain.


Utilisation
===========

Il faut d'abord créer un dictionnaire de mots compilé à partir d'une liste
de mots (non fournie !).

    # mots.txt est une liste de mots triée sans doublons
    $wordbrain -c mots.txt mots.tree 
    Compilation d'un dictionnaire de 187383 mots.

Résoudre ensuite les niveaux en utilisant ce dictionnaire.

    $wordbrain -d mots.tree oeuf niis cmll alba 6 5 5
    ****************************************************************************
    croc + goutte + gateau
    g  e  t  o      |    g  e6           |    g1              |    
    a  t  u  c1     |    a  t5 t4        |    a2              |    
    t  e  g  r2     |    t  e  u3        |    t3 e4           |    
    u  a  o3 c4     |    u  a  g1 o2     |    u6 a5           |    
    
    ****************************************************************************
    croc + goutte + gateau
    g  e  t  o      |    g  e            |    g1              |    
    a  t  u  c1     |    a  t5 t4        |    a2              |    
    t  e  g  r2     |    t  e6 u3        |    t3 e4           |    
    u  a  o3 c4     |    u  a  g1 o2     |    u6 a5           |    
    
    ****************************************************************************
    croc + goutte + gateau
    g  e  t  o      |    g  e6           |    g1              |    
    a  t  u  c1     |    a  t4 t5        |    a2              |    
    t  e  g  r2     |    t  e  u3        |    t3 e4           |    
    u  a  o3 c4     |    u  a  g1 o2     |    u6 a5           |    
    
    ****************************************************************************
    croc + goutte + gateau
    g  e  t  o      |    g  e            |    g1              |    
    a  t  u  c1     |    a  t4 t5        |    a2              |    
    t  e  g  r2     |    t  e6 u3        |    t3 e4           |    
    u  a  o3 c4     |    u  a  g1 o2     |    u6 a5           |    
    

A propos
========

Le site wordbrainsolver qui a toutes les solutions de ce jeu étant la première
réponse de Google à la requête Wordbrain, ce programme n'est pas forcément très
utile pour le joueur. Pour son auteur il a par contre été plus amusant de
l'écrire que de jouer au jeu.

Ce programme a été écrit dans la forme libre qui caractérise en général plus
les scripts en Perl que le C++: code surtout impératif, avec des classes quand
c'est pratique sans rentrer dans un modèle orienté objet, en utilisant des
librairies externes qui n'utilisent pas forcément les même conventions que le
programme (ici Qt). Ce style, pour moi adapté à la taille du problème n'est
donc pas forcément un bon exemple pour tout le monde ;)

Utilisant la "force brutale" (algorithme stupide) pour résoudre les niveaux, le
programme est raisonablement optimisé afin de pouvoir trouver des solutions
rapidement: pas d'allocations mémoire pendant la recherche, structuration
efficace des données, opérations de comparaison et de copie binaires.

Ce bout de code est distribué sous la license MIT (voir LICENSE.txt ci-joint)
qui vous autorise en gros à l'utiliser et le distribuer comme vous le voulez
tant que la note de copyright est laissée en place.

