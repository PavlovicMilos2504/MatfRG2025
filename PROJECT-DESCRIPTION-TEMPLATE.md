# Bilijarska sala

22008 - Milos Pavlovic  
3D scena bilijarske sale sa stolom, direkcionim i tackastim (lampa) svjetlom, bloom efektom i sjenkama.

## Controls

W, A, S, D -> kretanje kamere  
Mis -> rotacija kamere  
L -> paljenje/gasenje lampe iznad stola  
F1 -> prikaz/sakrivanje GUI-ja  
ESC -> izlazak iz aplikacije

## Features

### Fundamental:

[x] Model with lighting
[x] Two types of lighting with customizable colors and movement through GUI or ACTIONS
[x] Taster L (paljenje lampe) --- AFTER 1 SECOND ---Triggers---> Lampa se pali iznad stola ---> AFTER 2 SECONDS ---Triggers---> Lampa dostize punu jacinu svjetla

### Group A:

[ ] Frame-buffers with post-processing   
[ ] Off-screen Anti-Aliasing  
[ ] Parallax Mapping
[x] Bloom with the use of HDR

### Group B:
[ ] Deferred Shading  
[x] Point Shadows  
[ ] SSAO

### Engine improvement:

[x] Ispravno ucitavanje modela sa vise ugnijezdenih cvorova (Assimp aiProcess_PreTransformVertices)
[x] Ispravka PlatformController-a: JustPressed stanje tastera se sada racuna direktno iz GLFW akcije umjesto zastarjelog stanja

## Models:

[Billiard Bar](https://sketchfab.com/3d-models/billiard-bar-e2291b3de255401b8a04c5bffc446437)

## Textures

Teksture su ukljucene uz model bilijarske sale (vidi sekciju Models).
