# Güncel sürüm: 2D hareket demosu eklendi

## Çalıştır

1. Önceki uygulamayı kapat. Bu ZIP'i **yeni bir klasöre tamamen çıkar**.
2. İçindeki `hsv-kalman-lab` klasörünü aç. `START_WINDOWS.cmd` burada görünmeli.
3. Dosya Gezgini adres çubuğuna `powershell` yazıp Enter'a bas; ya da bu klasörü VS Code'da açıp yeni terminal aç.
4. Şunu çalıştır:

```powershell
.\START_WINDOWS.cmd -Motion
```

Mevcut CMake, Build Tools ve OpenCV kurulumun yeterli; yeniden kurulum gerekmez. İlk açılış yeni sürümü derleyip iki test programını çalıştırır. OpenCV yolu sorulursa `C:\opencv\build` yaz.

Yeni **3 — 2D Motion Simulation** sekmesi doğrudan açılır. Klavyedeki **L** ile iki saniyelik ölçüm kaybını, **P** ile farklı hareket yollarını dene. **Space** duraklatır, **R** yeniden başlatır. Aynı işlemler ekrandaki düğmelerle de yapılır.

- Dolu daire: gerçek sanal nesne.
- İnce renkli kutu: gürültülü ölçüm.
- Yeşil kutu ve iz: Kalman tahmini ve geçmişi.
- Çarpı: 0,4 saniye sonraki konum öngörüsü.
- Elips: tahminin modelden hesaplanan belirsizliği.

Bu bölüm sentetik verilerle çalışır; kamera renk maskesi 1. sekmede, sıcaklık grafiği 2. sekmededir. Sağdaki slider'lar 1. ve 2. sekmeler içindir.

## Hızlı GitHub yükleme — Git komutu gerektirmez

1. GitHub'da **New repository** aç. Önerilen ad: `hsv-kalman-lab`. CV'den erişilebilmesi için **Public** seç. README, .gitignore ve lisans oluşturma seçeneklerini boş bırak; dosyalar pakette hazır.
2. Yeni deponun boş sayfasında **uploading an existing file** bağlantısına tıkla. Depo doluysa **Add file → Upload files** yolunu kullan.
3. ZIP'i açınca gelen dış klasörü değil, **`hsv-kalman-lab` klasörünün içindekileri** yükleme alanına sürükle. `README.md` ve `CMakeLists.txt` depo kökünde kalmalı.
4. Uygulamayı daha önce derlediysen **`build-windows` veya diğer `build...` klasörlerini seçme**. Web üzerinden yüklemede `.gitignore` dosyası seçtiğin klasörü otomatik dışlamaz. OpenCV'nin `C:\opencv` klasörünü de yükleme.
5. Yüklenecek içerik: `src`, `tests`, `scripts`, `docs`, `.github`, `.gitignore`, `CMakeLists.txt`, `README.md`, `HIZLI_BASLANGIC_TR.md`, `CHANGELOG.md`, `START_WINDOWS.cmd`, `LICENSE`.
6. Commit açıklamasına `Add HSV and Kalman educational application` yaz ve **Commit changes** seç.
7. Depo ana sayfasında README'nin göründüğünü kontrol et. CV'ye deponun tam bağlantısını ekle. Kodlar henüz hesabına otomatik yüklenmedi.

README İngilizce sunuma hazır ve AI desteğini belirtiyor. Projeyi CV'de HSV renk maskeleme ve sentetik veride Kalman filtreleme uygulaması olarak anlat. Kamera üzerinden hareket takibi içerdiğini yazma.

Kaynak: https://docs.github.com/en/repositories/working-with-files/managing-files/adding-a-file-to-a-repository
