import urllib.request
import gzip
import os
from pathlib import Path

english_50MB = "https://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz"
english_100MB = "https://pizzachili.dcc.uchile.cl/texts/nlang/english.100MB.gz"
english_200MB = "https://pizzachili.dcc.uchile.cl/texts/nlang/english.200MB.gz"
english_1024MB = "https://pizzachili.dcc.uchile.cl/texts/nlang/english.1024MB.gz"

dna_50MB = "https://pizzachili.dcc.uchile.cl/texts/dna/dna.50MB.gz"
dna_100MB = "https://pizzachili.dcc.uchile.cl/texts/dna/dna.100MB.gz"
dna_200MB = "https://pizzachili.dcc.uchile.cl/texts/dna/dna.200MB.gz"

protein_50MB = "https://pizzachili.dcc.uchile.cl/texts/protein/proteins.50MB.gz"
protein_100MB = "https://pizzachili.dcc.uchile.cl/texts/protein/proteins.100MB.gz"
protein_200MB = "https://pizzachili.dcc.uchile.cl/texts/protein/proteins.200MB.gz"

datasets = {
    "english_50MB": english_50MB,
    "english_100MB": english_100MB,
    "english_200MB": english_200MB,
    "english_1024MB": english_1024MB,
    "dna_50MB": dna_50MB,
    "dna_100MB": dna_100MB,
    "dna_200MB": dna_200MB,
    "protein_50MB": protein_50MB,
    "protein_100MB": protein_100MB,
    "protein_200MB": protein_200MB,
}

def download_and_extract(url, output_name):
    gz_path = f"{output_name}.gz"
    output_path = output_name

    if os.path.exists(output_path):
        print(f"{output_name} already exists, skipping download")
        return

    print(f"Downloading {url}")
    urllib.request.urlretrieve(url, gz_path)

    print(f"Extracting {gz_path}")
    with gzip.open(gz_path, 'rb') as f_in:
        with open(output_path, 'wb') as f_out:
            f_out.write(f_in.read())

    os.remove(gz_path)
    print(f"Downloaded and extracted to {output_path}\n")

if __name__ == "__main__":
    datasets_dir = Path(__file__).parent / "datasets"
    datasets_dir.mkdir(exist_ok=True)

    os.chdir(datasets_dir)

    for name, url in datasets.items():
        download_and_extract(url, name)

    print("All datasets downloaded successfully")
