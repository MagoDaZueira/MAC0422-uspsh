import matplotlib.pyplot as plt
import sys
import math

def soma_quarta_coluna(arquivo):
    total = 0
    with open(arquivo, 'r') as file:
        for linha in file:
            partes = linha.strip().split()
            if len(partes) >= 4:
                try:
                    total += int(partes[3])
                except ValueError:
                    print(f"[!] Valor inválido em '{arquivo}': '{partes[3]}' ignorado.")
    return total

def qtde_preempcoes(arquivo):
    with open(arquivo) as f:
        # Lê todas as linhas, removendo quebras de linha e espaços
        linhas = [linha.strip() for linha in f if linha.strip()]

    ultimo_valor = int(linhas[-1])
    return ultimo_valor


def deadlines(arquivos, titulo):
    nomes_barras = ['FCFS', 'SRTN', 'Prioridade']
    nome_eixo_x = 'Escalonador'
    nome_eixo_y = 'Deadlines'

    # Calcula as somas
    somas = [soma_quarta_coluna(arq) for arq in arquivos]

    # Cria gráfico
    plt.figure(figsize=(8, 5))
    plt.bar(nomes_barras, somas, color=['#1f77b4', '#ff7f0e', '#2ca02c'])

    # Nomes dos eixos
    # plt.xlabel(nome_eixo_x)
    plt.ylabel(nome_eixo_y)
    plt.title(titulo)

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)

    plt.subplots_adjust(left=0.5)
    plt.ylim(0, 30)

    plt.tight_layout()
    plt.show()


def preempcoes(arquivos, titulo):
    nomes_barras = ['FCFS', 'SRTN', 'Prioridade']
    nome_eixo_x = 'Escalonador'
    nome_eixo_y = 'Preempções'

    valores = [qtde_preempcoes(arquivo) for arquivo in arquivos]

    # Cria gráfico
    plt.figure(figsize=(8, 5))
    plt.bar(nomes_barras, valores, color=['#1f77b4', '#ff7f0e', '#2ca02c'])

    # Nomes dos eixos
    # plt.xlabel(nome_eixo_x)
    plt.ylabel(nome_eixo_y)
    plt.title(titulo)

    max_soma = max(valores)
    limite_y = max(1, math.ceil(max_soma * 1.2))
    plt.ylim(0, 30)

    # Força o uso de inteiros no eixo Y
    plt.yticks(range(0, limite_y + 1, max(1, limite_y // 10)))

    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)

    plt.subplots_adjust(left=0.5)

    plt.tight_layout()
    plt.show()


def main(arquivos, titulo):
    if int(sys.argv[1]) == 1:
        deadlines(arquivos, titulo)
    elif int(sys.argv[1]) == 2:
        preempcoes(arquivos, titulo)

if __name__ == "__main__":
    titulo = sys.argv[2]
    titulo_com_quebra = titulo.replace('\\n', '\n')
    main(sys.argv[3:], titulo_com_quebra)
