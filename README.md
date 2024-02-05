-Объединяет структуру хранилища и сервер. Объединенный файл находится в server_save.c.

-Использование библиотеки sem_t из библиотеки semaphore для управления ресурсами (аналог блокировки).

-Текущий формат ввода следующий: сначала запускается клиент, указывается адрес и порт, затем вводятся необходимые команды (в одной строке).

-При первоначальном запуске клиента данные по умолчанию загружаются из файла save.save.

-Файл для тестирования многопоточности - test.c.

test0:
Добавьте данные в файл хранения, выполните программу в соответствии с инструкциями, затем проверьте, были ли данные успешно загружены. При этом проведите тестирование функций find, find_more, find_less и т. д., чтобы убедиться в их корректной работе.
![1707140264254](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/fa84b696-ffa8-477d-b9bb-9b6821178869)

![1707140856148](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/29a09de5-136b-4e0d-b605-1f8993e402b1)

![1707141169461](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/c556fd88-9b7a-4e16-b384-e83a40fef6c5)

test1：
Проведите тестирование операций удаления, поиска после удаления, повторного добавления после удаления, поиска после повторного добавления и поиска несуществующего ключа.
Выполните операции сохранения и обновления данных, затем проверьте, были ли данные корректно обновлены и сохранены в файл.
![1707141508481](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/d9cda755-ba93-4a75-8685-8e5fbbe97687)

![1707140943693](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/d326ec6a-c1b0-4f59-84a1-8d1d51afdc2a)

![1707140984894](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/4ac8dc32-37d4-4d9d-a2ee-42c2fa72b37e)

![1707141004496](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/c3687e11-4c6d-4847-8747-35b1921b41d1)

test2：
Напишите код, используя многопоточность, для моделирования одновременных операций нескольких пользователей.

![1706879105442](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/9f32573c-a771-4537-b88d-b89866e1871d)

![1707142451610](https://github.com/Zheng-Niannian/KVDB_C/assets/79322267/c34aa0ec-39eb-4e60-bbff-8fe9a5645cc9)
