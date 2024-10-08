# SDxMIK32
Это репозиторий драйвера SD-карт и файловой системы MIK32FAT, совместимой с файловой системой FAT32.
Репозиторий построен как проект PlatformIO IDE для микроконтроллера MIK32, framework - framework-mik32v2-sdk. Файл main.c содержит пример записи и чтения файлов. Файлы mik32fat.h и mik32fat.c относятся к файловой системе. Файлы sd.h и sd.c - это драйвер диска.

## **Драйвер карт SD**
- Поддерживает SDHC-карты информационной емкостью до 32ГБ
- Реализует общение с картами SD через шину SPI
- Поддерживает чтение и запись данных через DMA (опция включена по умолчанию, для отключения надо закомментировать строку '#define SD_DRIVER_USE_DMA' в файле sd.h)

Драйвер SD-карт в настоящее время реализует общение с картами по шине SPI. Имеются функции инициализации карты SD (с попутным определением ее типа), функции отправки команды на карту, а также функции чтения, записи и стирания 512-байтных секторов.

## **Файловая система MIK32FAT**

Позволяет читать, дозаписывать и перезаписывать текстовые и бинарные файлы, создавать и удалять файлы и директории. Файловая система в настоящее время не поддерживает формат длинных имен файлов (LFN), имена открываемых и создаваемых файлов и директорий должны состоять из заглавных (!) латинских букв и цифр, длина имени не должна превышать 8 символов, а длина расширения -- 3 символов.

Перед началом работы с картой памяти убедитесь, что она форматирована как FAT32.
Во время инициализации файловой системы (функция MIK32FAT_Init) выполняется чтение boot-сектора диска, открывается первый том диска из существующих. В ходе инициализации в дескриптор файловой системы загружаются значения номеров начальных секторов секции данных, записей таблицы аллокации файлов (FAT). После выполнения функции MIK32FAT_Init микроконтроллер готов к работе с файловой системой.

`FAT_Status_t MIK32FAT_Init(FAT_Descriptor_t* fs);`

- Поле fs - указатель на дескриптор файловой системы (структура типа FAT_Descriptor_t)
### **Работа с файлами**

#### **Открытие файлов**

Работа с файлами начинается с вызова функции MIK32_FileOpen

`FAT_Status_t MIK32FAT_FileOpen(FAT_File_t* file, FAT_Descriptor_t* fs, char* name, char modificator);`

- Поле file - указатель на структуру-дескриптор файла

- Поле fs - указатель на структуру-дескриптор файловой системы

- Поле name - путь (имя) файла

- Поле modificator определяет режим доступа к файлу

Всякая функция файловой системы требует для работы информацию из структуры-дескриптора файловой системы, однако функции работы с файлами требуют также доступ к специфической информации о файле, что есть только в структуре-дескрипторе файла. Чтобы не подавать аргументом функции два указателя на структуры-дескрипторы, указатель на дескриптор файловой системы включается в дескриптор файла функцией MIK32FAT_FileOpen.

Функция MIK32FAT_FileOpen, помимо этого, выполняет поиск файла на диске по строке-пути. Строка путь (поле name)

- совпадает с именем файла, если файл находится в корневом каталоге диска

- содержит перед имененм файла названия субдиректорий, отделенные символом '/' ("слеш")

Так, например, если необходимо прочитать файл TEXT.TXT, который находится в папке FOLDER, строка-путь должна иметь вид "FOLDER/TEXT.TXT".

Поле modificator имеет 3 возможных значения

- 'R' (0x52) -- открыть файл для чтения

- 'W' (0x57) -- открыть файл для записи (дозаписи). Если открываемый файл не будет найден на диске, он будет автоматически создан

- 'A' (0x41) -- открыть файл для перезаписи. Если открываемый файл не будет найден на диске, он будет автоматически создан. Если файл будет найден, он будет удален, а затем автоматически создан.

Если файл был открыт для чтения ('R'), то к нему применим метод MIK32FAT_ReadFile. Если файл был открыт для дозаписи или для перезаписи ('W' или 'A'), то к нему применим метод MIK32FAT_WriteFile.

#### **Чтение файлов**

Чтение файлов выполняется функцией MIK32FAT_ReadFile.

`uint32_t MIK32FAT_ReadFile(FAT_File_t* file, char* buf, uint32_t quan)`

- Поле file - указатель на структуру-дескриптор файла

- Поле buf - указатель на массив, куда данные будут записываться

- Поле quan - количество читаемых байт файла

Функция возвращает количество прочитанных байт. Если файл был открыт для записи или перезаписи, а его пытаются читать, функция выдадет 0.

#### **Запись файлов**

Запись файлов выполняется функцие MIK32FAT_WriteFile.

`uint32_t MIK32FAT_WriteFile(FAT_File_t* file, const char* buf, uint32_t quan);`

- Поле file - указатель на структуру-дескриптор файла

- Поле buf - указатель на массив, откуда данные будут браться для записи

- Поле quan - количество записываемых байт файла

Функция возвращает количество записанных байт. Если файл был открыт для чтения, а в него пытаются что-то записать, функция выдадет также 0.

#### **Закрытие файлов**

Дескриптор файловой системы содержит 512-байтный буфер сектора диска. Функция MIK32FAT_WriteFile выполняет запись содержимого буфера на диск только когда буфер заполняется полностью. Если после вызова MIK32FAT_WriteFile количество байт в буфере меньше его длины, информация не будет перенесена на диск. Чтобы дозаписать остатки данных на диск, существует функция MIK32FAT_FileClose.

`FAT_Status_t MIK32FAT_FileClose(FAT_File_t* file)`

### **Прочие функции файловой системы**

#### **Создание файлов и директорий**

Для создания файлов и директорий существует функция MIK32FAT_Create.

`FAT_Status_t MIK32FAT_Create(FAT_Descriptor_t* fs, char* name, bool dir)`

- Поле fs - указатель на дескриптор файловой системы

- Поле name - путь до файла или директории (смотри Открытие файлов)

- Поле dir - если true, то создаем директорию, если false, то создаем новый пустой файл

#### **Удаление файлов и директорий**

Удаление файлов и директорий осуществляется функцией MIK32FAT_Delete.

`FAT_Status_t MIK32FAT_Delete(FAT_Descriptor_t* fs, char* name)`

- Поле fs - указатель на дескриптор файловой системы

- Поле name - путь до файла или директории (смотри Открытие файлов)

Важно отметить, что зачастую, когда при использовании персонального компьютера пользователь нажимает на кнопку удаления файла, файл не удаляется, а перемещается в скрытую директорию TRASH, которая расположена в корневом каталоге. Функция же MIK32FAT_Delete именно удаляет файл, стирая его с диска.
