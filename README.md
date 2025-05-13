# Projeto de Computação Distribuída: Implementação de Torrent P2P

Projeto para a disciplina Computação Distribuída, focando na implementação de um sistema de compartilhamento de arquivos Peer-to-Peer (P2P) utilizando o protocolo BitTorrent. O projeto é dividido em dois componentes principais: um servidor tracker e um cliente peer. Juntos, eles permitem a criação, compartilhamento e download de arquivos de forma distribuída.

Grupo:
   - Caio Prá Silva
   - Lucas Costa Valença
   - Pedro Nack Martins
   - Rafael Luis Sol Veit Vargas

## Estrutura do Projeto

- **tracker/**: Contém a implementação do servidor tracker escrito em Rust.
  - `main.rs`: O ponto de entrada principal para o servidor tracker.
  - `peer.rs`: Define a struct `Peer` e fornece utilitários para codificar informações de peers.
  - `Cargo.toml`: Arquivo de configuração do projeto Rust.
  - `.env`: Configuração de ambiente para logging.
- **peer/**: Contém a implementação do cliente peer escrito em C++.
  - `peer.h` e `peer.cpp`: Definem a classe `Peer` e suas funcionalidades.
  - `main.cpp`: O ponto de entrada principal para o cliente peer.
- **torrents/**: Armazena arquivos torrent gerados.
- **downloads/**: Armazena arquivos baixados pelo cliente peer.
- **build/**: Arquivos resultantes da construção do executável de `Peer`.

## Servidor Tracker

O servidor tracker é responsável por coordenar a comunicação entre os peers. Ele mantém uma lista de peers ativos para cada arquivo compartilhado e fornece essas informações aos peers mediante solicitação. O servidor é implementado utilizando o framework Actix Web em Rust.

### Principais Funcionalidades
- Lida com requisições `/announce` de peers.
- Rastreia peers ativos e remove entradas desatualizadas.
- Responde com uma lista compacta de peers utilizando o formato de codificação bencoding do BitTorrent.

### Como Funciona
1. Um peer envia uma requisição HTTP GET para o endpoint `/announce` com parâmetros como `info_hash`, `peer_id` e `port`.
2. O tracker atualiza sua lista de peers para o `info_hash` fornecido.
3. O tracker responde com uma lista de peers ativos para o arquivo solicitado.

## Cliente Peer

O cliente peer é responsável por semear e baixar arquivos. Ele utiliza a biblioteca libtorrent para lidar com operações relacionadas a torrents.

### Principais Funcionalidades
- Cria arquivos torrent para arquivos locais.
- Baixa arquivos utilizando arquivos `.torrent`.
- Gerencia torrents ativos e exibe seu status.

### Como Funciona
1. **Seeding**: O cliente cria um arquivo torrent para um arquivo local e começa a semeá-lo. O arquivo torrent inclui metadados como tamanho do arquivo, tamanho das partes e URL do tracker.
2. **Download**: O cliente baixa um arquivo utilizando um arquivo `.torrent`. Ele se conecta ao tracker para obter uma lista de peers e baixa partes do arquivo deles.
3. **Comandos CLI**: O cliente fornece uma interface de linha de comando para ações como `seed`, `download`, `stop` e `list`.

## Comunicação Entre Peers

Os peers se comunicam indiretamente através do servidor tracker. O tracker fornece uma lista de peers compartilhando um arquivo específico, e os peers então estabelecem conexões diretas para trocar partes do arquivo. Essa abordagem descentralizada garante escalabilidade e tolerância a falhas.

## Como Executar

### Servidor Tracker
1. Navegue até o diretório `tracker/`.
2. Execute o servidor utilizando o Cargo (com a flag *--release*, opcionalmente):
   ```bash
   cargo run [--release]
   ```
3. O servidor será iniciado em `http://0.0.0.0:8080`.

### Cliente Peer
1. Compile o projeto utilizando o CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
2. Execute o cliente peer:
   ```bash
   cd ..
   ./build/peer
   ```
3. Use a CLI para semear ou baixar arquivos.

## Exemplo de Fluxo de Trabalho
1. Inicie o servidor tracker.
2. Use o cliente peer para semear um arquivo (recomenda-se criar uma pasta na raíz do projeto, como *images/*):
   ```
   seed <file_path>
   ```
3. Use outra instância do cliente peer para baixar o arquivo utilizando o arquivo `.torrent` gerado:
   ```
   download <torrent_path>
   ```

## Dependências

### Servidor Tracker
- Rust
- actix-web
- dotenv
- env_logger
- tokio
- serde

### Cliente Peer
- C++
- libtorrent
- CMake

## Notas
- Certifique-se de que o servidor tracker está em execução antes de semear ou baixar arquivos.
- O tracker e o cliente peer se comunicam via HTTP utilizando o protocolo BitTorrent.
