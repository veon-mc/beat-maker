## Getting Started

1. Clone this repository locally.
2. From the root folder of the clone, run the command below to fetch
   the Git submodule containing the precompiled libraries.
```
$ git submodule update --init libraries
```

*NOTE:* Ce repo contient un fichier .gitignore limitant les commits git 
aux fichiers sources. Il est normal d'avoir à recréer les configurations 
d'exécution dans CLion à la première ouverture du dossier en tant que 
projet. Il en va probablement de même pour les autres IDE.
