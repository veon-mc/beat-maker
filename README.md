## Premiers pas

1. Clonez ce dépot git en local.
2. Dans le dossier raçine du clone, invoquez la commande reprise ci-dessous
pour intégrer le sous-module Git contenant les librairies précompilées.

```
$ git submodule update --init libraries
```

*NOTE:* Ce repo contient un fichier .gitignore limitant les commits git 
aux fichiers sources. Il est normal d'avoir à recréer les configurations 
d'exécution dans CLion à la première ouverture du dossier en tant que 
projet. Il en va probablement de même pour les autres IDE.
