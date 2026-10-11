# Windows / Linux kurulum ayrıntıları

**v1.1 güncellemesi:** Üçüncü sekmede iki boyutlu sentetik hareket demosu vardır. `.\START_WINDOWS.cmd -Motion` ile doğrudan açılır. Güncel özellikler için [README](../README.md), hızlı yükseltme ve GitHub yükleme için [Türkçe başlangıç](../HIZLI_BASLANGIC_TR.md) dosyasına bak. Aşağıdaki ayrıntılar ilk iki deneyin kurulumu ve kullanımı içindir.

C++17 ve OpenCV 4 ile iki bağımsız eğitim deneyi: **USB webcam üzerinde HSV renk maskeleme** ve **sentetik sıcaklık sensörü için tek boyutlu Kalman filtresi**.

Bu sürüm renk seçimini görselleştirir; nesne kimliği, hareket takibi veya kamera koordinatlarından Kalman tahmini üretmez. Kalman bölümü yalnızca kendi ürettiği sıcaklık verisini işler.

## Özellikler

- İki sekmeli masaüstü arayüzü; klavye veya fareyle sekme seçimi.
- Webcam seçimi, yeniden bağlanma ve ayna görünümü.
- Altı HSV eşiği, renk çemberini aşan H aralığı ve morfolojik temizleme.
- Orijinal görüntü, ikili maske, seçili piksel yüzdesi.
- Webcam gerektirmeyen sentetik renk kartelası.
- Gürültülü sıcaklık ölçümleri, gerçek sinyal ve filtre çıktısının aynı grafikte gösterimi.
- Gürültü, Q ve R ayarları; RMSE karşılaştırması, duraklatma ve sıfırlama.
- Simülasyonun görünür son 300 örneğini CSV olarak dışa aktarma.
- CMake, testler, GitHub Actions ve MIT lisansı.

Kamera görüntüsü kaydedilmez veya bir sunucuya gönderilmez. CSV yalnızca simülasyon verisidir ve `E` tuşuna basıldığında yazılır.

## Windows 10 / 11 — ilk kurulum

Paket **kaynak kod içerir; hazır Windows EXE içermez**. İlk kullanımda aşağıdaki üç bileşen gerekir. Python gerekmez. GPU/CUDA gerekmez.

1. [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/vs/older-downloads/) veya Visual Studio 2022 Community kur. Kurulumda **Desktop development with C++ / C++ ile masaüstü geliştirme** iş yükünü, MSVC v143 x64 ve Windows SDK bileşenlerini seç. Visual Studio Code tek başına derleyici değildir. Betik açıkça VS 2022 kullanır.
2. [CMake](https://cmake.org/download/) kur. **Add CMake to the system PATH** seçeneğini etkinleştir. Kurulumdan sonra terminali kapatıp aç.
3. [OpenCV resmi sürümlerinden](https://opencv.org/releases/) bir **OpenCV 4.x Windows** paketi indirip çıkar. Örneğin `C:\opencv\build\OpenCVConfig.cmake` dosyası oluşmalı. Kaynak kod ZIP'i yerine derlenmiş Windows paketini kullan; 4.12.0 uygun bir başlangıç sürümüdür. Betik resmi paketteki x64 runtime DLL'lerini uygulamanın yanına kopyalar.
4. Bu projenin ZIP'ini tamamen çıkar; örneğin `C:\Projects\hsv-kalman-lab`. ZIP içinden çalıştırma.
5. **`START_WINDOWS.cmd` dosyasına çift tıkla.** İlk açılışta derler, testleri çalıştırır ve uygulamayı açar. Sonraki açılışlarda CMake yalnızca gerekli değişiklikleri derler.

OpenCV başka klasördeyse betik `OpenCVConfig.cmake` dosyasının bulunduğu klasörü sorar. Örneğin `D:\tools\opencv\build`. Derleme sonunda uygulama `build-windows\Release\hsv_kalman_lab.exe` yolundadır; DLL'leri yanında bırak. Bundan sonra EXE'ye doğrudan çift tıklayabilirsin.

### Kamera ve demo seçenekleri

Proje klasöründe PowerShell aç:

```powershell
# Kamera olmadan renk kartelasıyla başlat
.\START_WINDOWS.cmd -Demo

# Dahili kamera yerine USB webcam; indeks cihazına göre 0, 1 veya 2 olabilir
.\START_WINDOWS.cmd -Camera 1

# Yalnızca simülasyon sekmesinde başlat
.\START_WINDOWS.cmd -Simulation

# OpenCV başka yerdeyse
.\START_WINDOWS.cmd -OpenCVRoot "D:\tools\opencv" -Camera 1
```

`START_WINDOWS.cmd` kendi PowerShell sürecinde betiği çalıştırır; makinenin kalıcı execution policy ayarını değiştirmez.

## Ubuntu / Debian

Grafik masaüstü oturumunda:

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/hsv_kalman_lab --camera 0
```

Kamerasız: `./build/hsv_kalman_lab --demo`. Sadece simülasyon: `./build/hsv_kalman_lab --simulation`.

## Kullanım

Ana pencereyle birlikte **Lab Controls** penceresi açılır. Slider'lar bu ikinci pencerededir. Kısayollar için görüntü penceresine tıkla. Pencerelerden birini kapatmak uygulamayı sonlandırır. Ana pencere yaklaşık 1180 × 790 pikseldir; küçük ekranlarda Windows ekran ölçeklemesini azaltmak gerekebilir.

| Tuş | İşlev |
|---|---|
| `1` / `2` | HSV / Kalman sekmesi |
| `N` | HSV'de sonraki kamera, 0–5 arasında döner |
| `O` | HSV'de aynı kamerayı yeniden aç |
| `D` | HSV'de demo kartelası / webcam |
| `M` | HSV'de ayna görüntüsünü aç/kapat |
| `Space` | Kalman simülasyonunu duraklat/sürdür |
| `R` | Kalman simülasyonunu ve metrikleri sıfırla |
| `E` | Görünür simülasyon örneklerini `simulation.csv` dosyasına yaz; önceki dosyanın üzerine yazar |
| `Esc` / `Q` | Çıkış |

### HSV deneyinin anlamı

OpenCV'nin 8 bit HSV dönüşümünde H **0–179**, S ve V **0–255** aralığındadır. Başlangıçta yeşil tonları seçilidir. H renk tonunu, S doygunluğu, V parlaklığı belirler. H alt sınırı üst sınırdan büyükse seçim çemberin sonundan başına sarar. S/V sınırları ters çevrilirse küçük olan alt sınır olarak kullanılır. `Clean radius=0` temizlemeyi kapatır; diğer değerler önce opening sonra closing uygular. Işık değişince eşikleri yeniden ayarlamak gerekir. Maskedeki beyaz bölgeler renk koşulunu sağlayan piksellerdir; bunlar bir nesnenin tanındığı anlamına gelmez.

### Kalman deneyinin anlamı

Simüle edilen sıcaklık yavaşça değişir ve her 20 saniyede bir basamak değişimi içerir. Ölçüme sıfır ortalamalı Gauss gürültüsü eklenir. Rastgele sayı tohumu 42'dir; aynı parametrelerle sıfırlanan deney aynı ortamda tekrar üretilebilir.

Model: `x(k) = x(k-1) + w(k)` ve `z(k) = x(k) + v(k)`.

```
P_prior = P + Q
K = P_prior / (P_prior + R)
x = x + K * (z - x)
P = (1-K)^2 * P_prior + K^2 * R
```

- `Noise x0.1`: gerçek ölçüm gürültüsünün standart sapması (°C).
- `Q x0.001`: modelin adım başına süreç varyansı (°C²); yüksek Q yeni ölçümlere daha hızlı uyum sağlar.
- `R x0.1`: filtrenin varsaydığı ölçüm gürültüsü varyansı (°C²); yüksek R daha fazla yumuşatma ve gecikme oluşturabilir.
- Sıfıra getirilen Q/R slider değerleri hesapta 1 kabul edilir.
- Varsayılan standart sapma 2 °C olduğundan R=4 °C² seçilidir. Q=0.01 °C²'dir.
- Her simülasyon örneği 0.1 saniyeyi temsil eder. Yoğun sistem yükünde simülasyon gerçek zamandan yavaş ilerleyebilir; adım atlamaz.
- RMSE, son sıfırlamadan itibaren tüm örneklerden hesaplanır. Grafik ve CSV son 300 örneği kapsar. Grafik 5–45 °C dışında kalan değerleri görsel olarak sınırlar; RMSE/CSV gerçek değerleri kullanır.
- Gürültü veya Q/R değiştiğinde deney ve metrikler otomatik sıfırlanır. Parametrelere göre filtre çıktısı ham ölçümden kötü de olabilir; düşük RMSE garanti edilmez.

## Sorun giderme

| Durum | Yapılacak işlem |
|---|---|
| `cmake bulunamadi` | CMake'i PATH seçeneğiyle kur, yeni terminal aç. |
| `Visual Studio 17 2022` bulunamadı | VS 2022 + Desktop development with C++ iş yükünü kur. Başka sürüm kullanacaksan betikteki generator satırını kurulu sürüme uyarlaman gerekir. |
| `OpenCVConfig.cmake` bulunamadı | Derlenmiş Windows paketini çıkar; istenen klasöre dosyanın bulunduğu yolu gir. |
| `opencv_world...dll` eksik | START_WINDOWS.cmd ile yeniden başlat; EXE'yi DLL'lerinden ayırma. |
| Kamera açılmadı / siyah görüntü | Windows Ayarlar → Gizlilik ve güvenlik → Kamera → Masaüstü uygulamaları için erişimi aç. Teams/Zoom ve diğer kamera uygulamalarını kapat. N veya O tuşunu dene. |
| Yanlış kamera | N ile değiştir veya `-Camera 1` kullan. İndeks kalıcı cihaz kimliği değildir. |
| Maske tamamen siyah | Demo modu ile kontrol et; H aralığını genişlet, S/V alt eşiklerini azalt. |
| Linux'ta kamera izin hatası | `/dev/video*` aygıt izinlerini ve hesabının video grubuna erişimini kontrol et; uygulamayı root olarak çalıştırma. |
| Grafik arayüz açılmıyor | Headless OpenCV yerine HighGUI/GTK/Qt destekli OpenCV ve grafik masaüstü oturumu gerekir. |

## GitHub'a yükleme

GitHub'da `hsv-kalman-lab` adında **boş** bir repository oluştur. README/lisans oluşturma seçeneklerini seçme; bunlar zaten projede var. Ardından proje klasöründe:

```bash
git init
git add .
git commit -m "Add HSV webcam explorer and scalar Kalman simulation"
git branch -M main
git remote add origin https://github.com/KULLANICI_ADIN/hsv-kalman-lab.git
git push -u origin main
```

`KULLANICI_ADIN` yerine kendi hesabını yaz. GitHub oturum açmanı isteyebilir. Paket senin hesabına otomatik yüklenmemiştir. Derleme klasörleri, DLL/EXE ve CSV `.gitignore` ile hariç tutulur. GitHub Actions gönderimden sonra Linux'ta uygulamayı derler ve test eder; bir Windows webcam testi değildir. Projenin README'sine kendi bilgisayarındaki çalışma ekran görüntülerini ve ölçümlerini ekleyebilirsin.

## Doğrulama durumu

Teslim ortamında `g++ -std=c++17 -Wall -Wextra -Werror` ile bağımsız Kalman testleri derlenip çalıştırıldı: ilk ölçüm, sabit sinyal, reset, sonlu/pozitif kovaryans, hatalı R ve sabit sıcaklıkta gürültü azaltma başarılı. Sabit sıcaklık testinde filtre/ham toplam karesel hata oranı yaklaşık **0.0196** bulundu; bu sonuç canlı uygulama veya basamaklı sinyal performansı iddiası değildir.

Teslim ortamında OpenCV geliştirme kitaplıkları, Windows ve USB webcam bulunmadığı için tam masaüstü uygulaması derlenip donanımla doğrulanamadı. Sağlanan GitHub Actions iş akışı da henüz çalıştırılmadı. İlk yerel derleme ve kamera kontrolü gereklidir.

## Kod yapısı

```
src/main.cpp               Arayüz, webcam, HSV maskesi ve simülasyon grafiği
src/kalman.hpp             Bağımsız skaler Kalman filtresi
tests/kalman_tests.cpp     Filtrenin deterministik testleri
scripts/windows.ps1       Windows derleme ve başlatma
START_WINDOWS.cmd         Çift tıklayarak başlatıcı
CMakeLists.txt            Çapraz platform derleme
.github/workflows/build.yml  Linux CI
```

## Kaynaklar

- [OpenCV inRange ve HSV](https://docs.opencv.org/4.12.0/da/d97/tutorial_threshold_inRange.html)
- [OpenCV Windows kurulumu](https://docs.opencv.org/4.12.0/d3/d52/tutorial_windows_install.html)
- [OpenCV KalmanFilter referansı](https://docs.opencv.org/4.11.0/dd/d6a/classcv_1_1KalmanFilter.html) — bu projede skaler filtre, denklemleri görünür tutmak için doğrudan C++ ile yazıldı.

## English overview

A C++17/OpenCV educational desktop lab with two independent views: an adjustable webcam HSV mask, and a scalar Kalman filter applied to a synthetic temperature sensor. The views do not share measurements. Includes CMake, a Windows build launcher, standalone numerical tests, and Linux CI. Source-only distribution; requires OpenCV 4 and a C++17 toolchain. See the Turkish instructions above for Windows setup and keyboard controls.
