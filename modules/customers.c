// Features for customer page

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include "customers.h"
#include "components/label.h"
#include "components/box.h"
#include "components/search.h"
#include "components/grid.h"
#include "components/button.h"
#include "components/scrolled.h"
#include "../ui/utils/get_last_iter.h"
#include "../ui/utils/search_in_model.h"
#include "../ui/utils/freeMemory.h"
#include "../ui/utils/update_txt.h"


void clear_grid_history(GtkGrid *grid_history) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid_history));
    GList *l;

    for (l = children; l != NULL; l = l->next) {
        GtkWidget *widget = GTK_WIDGET(l->data);

        // Nếu widget nằm ở dòng >= 1 (giả định tiêu đề ở dòng 0), ta cần gán thêm thông tin row khi thêm widget
        // Dùng gtk_widget_get_name() để kiểm tra
        const gchar *name = gtk_widget_get_name(widget);
        if (g_strcmp0(name, "title") != 0) {
            gtk_widget_destroy(widget);
        }
    }

    g_list_free(children);
}

void search_customer_history(GtkEntry *entry, gpointer user_data)
{
    HistoryData *h_data = (HistoryData *)user_data;

    const gchar *customer_id = gtk_entry_get_text(entry);
    if (strlen(customer_id) == 0) {
        clear_grid_history(h_data->grid_history);
        return;
    }

    // Xóa toàn bộ dữ liệu cũ trong grid_history (ngoại trừ tiêu đề)
    clear_grid_history(h_data->grid_history);

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(h_data->billingList), &iter);
    int row = 1;

    while (valid) {
        gchar *bill_id, *time, *cus_id, *service_id;
        gtk_tree_model_get(GTK_TREE_MODEL(h_data->billingList), &iter,
                           0, &bill_id,
                           1, &time,
                           2, &cus_id,
                           3, &service_id,
                           -1);

        if (g_strcmp0(customer_id, cus_id) == 0) {
            // Tìm tên dịch vụ và giá từ serviceList
            gchar *service_name = NULL;
            gchar *cost = NULL;

            GtkTreeIter service_iter;
            gboolean found = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(h_data->serviceList), &service_iter);
            while (found) {
                gchar *sid;
                gtk_tree_model_get(GTK_TREE_MODEL(h_data->serviceList), &service_iter, 0, &sid, -1);
                if (g_strcmp0(sid, service_id) == 0) {
                    gtk_tree_model_get(GTK_TREE_MODEL(h_data->serviceList), &service_iter,
                                       1, &service_name,
                                       2, &cost,
                                       -1);
                    g_free(sid);
                    break;
                }
                g_free(sid);
                found = gtk_tree_model_iter_next(GTK_TREE_MODEL(h_data->serviceList), &service_iter);
            }

            // Thêm dòng vào grid_history
            GtkWidget *label_time = gtk_label_new(time);
            GtkWidget *label_service = gtk_label_new(service_name ? service_name : "Không rõ");
            GtkWidget *label_cost = gtk_label_new(cost ? cost : "N/A");

            gtk_grid_attach(GTK_GRID(h_data->grid_history), label_time, 0, row, 1, 1);
            gtk_grid_attach(GTK_GRID(h_data->grid_history), label_service, 1, row, 1, 1);
            gtk_grid_attach(GTK_GRID(h_data->grid_history), label_cost, 2, row, 1, 1);
            row++;

            g_free(service_name);
            g_free(cost);
        }

        g_free(bill_id);
        g_free(time);
        g_free(cus_id);
        g_free(service_id);

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(h_data->billingList), &iter);
    }

    gtk_widget_show_all(GTK_WIDGET(h_data->grid_history));
}

/**
 * Thêm dữ liệu vào list store
 * @param store List store
 * @param ...args Các dữ liệu
 * @param filename tên của file text cần ghi thông tin
 */
static void addData(GtkListStore *store, const char *filename, ...)
{
    // Thêm dữ liệu vào ListStore
    va_list args;
    va_start(args, filename);

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set_valist(store, &iter, args);

    va_end(args);

    // Thêm dữ liệu vào file customers.txt
    updateTXT(store, filename);
}

static void on_save_clicked(GtkButton *button, gpointer add_data) {
    AddCustomerData *data = (AddCustomerData *)add_data;
    // Tạo id cho khách hàng mới thêm
        // Lấy id của khách hàng cuối cùng của Liststore
        gchar *last_id;
        GtkTreeIter last_iter; 
        get_last_iter(GTK_TREE_MODEL(data->store), &last_iter);
        gtk_tree_model_get(GTK_TREE_MODEL(data->store), &last_iter, 0, &last_id, -1);
        // Tạo id mới nhất
        const gchar *prefix = "KH";
        int number = atoi(last_id + strlen(prefix)); /* last_id là con trỏ tới id => +2 thì con trỏ sẽ dịch về sau 2 
        phần tử => trỏ tới "00x". atoi là hàm biến chuỗi số thành số nguyên => lấy được 00x = x */ 
        number++;
        gchar new_id[10];
        g_snprintf(new_id, sizeof(new_id), "%s%03d", prefix, number);

    // Lấy các giá trị đã nhập
    //const gchar *id = gtk_entry_get_text(GTK_ENTRY(data->id_entry));
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(data->name_entry));
    const gchar *phone = gtk_entry_get_text(GTK_ENTRY(data->phone_entry));
    const gchar *plate = gtk_entry_get_text(GTK_ENTRY(data->plate_entry));
    const gchar *type = gtk_entry_get_text(GTK_ENTRY(data->type_entry));

    // Thêm dữ liệu vào Liststore và vào file
    addData(data->store, "../database/customers.txt",
        0, new_id,
        1, name,
        2, phone,
        3, plate,
        4, type,
        -1);

    // Đóng cửa sổ sau khi lưu dữ liệu
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    gtk_widget_destroy(window);
}

static void on_delete_button_clicked(GtkWidget *widget, gpointer user_data)
{
    FindIterOfSearch *data = (FindIterOfSearch *)user_data;

    // Kiểm tra xem iter đã được tìm thấy chưa (tránh lỗi khi nhấn delete mà chưa tìm)
    if (data->result_iter != NULL)
    {
        gtk_list_store_remove(data->list_store, data->result_iter);
        g_print("Đã xóa khách hàng.\n");
    }
    // Update dữ liệu cho file txt
    updateTXT(data->list_store, "../database/customers.txt");

    // Đóng cửa sổ sau khi lưu dữ liệu
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(widget));
    gtk_widget_destroy(window);
}

static void on_edit_button_clicked(GtkButton *button, gpointer user_data) {
    FindIterOfSearch *data = (FindIterOfSearch *)user_data;

    // Lấy dữ liệu mới từ GtkEntry
    // const gchar *new_id = gtk_entry_get_text(GTK_ENTRY(data->id_entry));
    const gchar *new_name = gtk_entry_get_text(GTK_ENTRY(data->name_entry));
    const gchar *new_phone = gtk_entry_get_text(GTK_ENTRY(data->numberphone_entry));
    const gchar *new_plate = gtk_entry_get_text(GTK_ENTRY(data->numberplate_entry));
    const gchar *new_type = gtk_entry_get_text(GTK_ENTRY(data->cartype_entry));

    // Cập nhật lại dữ liệu trong GtkListStore
    if (data->result_iter != NULL) {
        gtk_list_store_set(data->list_store, data->result_iter,
            // 0, new_id,
            1, new_name,
            2, new_phone,
            3, new_plate,
            4, new_type,
            -1);
        
        g_print("Khách hàng đã được chỉnh sửa thành công!\n");
    } else {
        g_print("Không tìm thấy dòng cần chỉnh sửa!\n");
    }

    // Update dữ liệu cho file txt
    updateTXT(data->list_store, "../database/customers.txt");

    // Đóng cửa sổ sau khi lưu dữ liệu
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(button));
    gtk_widget_destroy(window);
}
// Feature add infomation of customers
void addCustomers(GtkWidget *widget, gpointer user_data) {
    CustomerData *data = (CustomerData *)user_data;

    // Thiết lập cửa sổ
    GtkWidget *addCustomers_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(addCustomers_window), "Thêm khách hàng");
    gtk_window_set_transient_for(GTK_WINDOW(addCustomers_window), GTK_WINDOW(data->main_window));  // Gắn vào cửa sổ chính
    gtk_window_set_modal(GTK_WINDOW(addCustomers_window), TRUE);  // Làm cửa sổ phụ modal
    gtk_window_set_default_size(GTK_WINDOW(addCustomers_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(addCustomers_window), GTK_WIN_POS_CENTER);

    // Tạo Grid
    GtkWidget *grid = createGrid(addCustomers_window);

    // Tạo các label
    // GtkWidget *id_label = gtk_label_new("Mã KH:");
    GtkWidget *name_label = gtk_label_new("Tên KH:");
    GtkWidget *numberphone_label = gtk_label_new("SĐT:");
    GtkWidget *numberplate_label = gtk_label_new("Biển số:");
    GtkWidget *cartype_label = gtk_label_new("Loại xe:");

    // Đặt các label vào grid
    // gtk_grid_attach(GTK_GRID(grid), id_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_label, 0, 4, 1, 1);

    // Tạo các entry
    // GtkWidget *id_entry =  gtk_search_entry_new();
    GtkWidget *name_entry =  gtk_search_entry_new();
    GtkWidget *numberphone_entry =  gtk_search_entry_new();
    GtkWidget *numberplate_entry =  gtk_search_entry_new();
    GtkWidget *cartype_entry =  gtk_search_entry_new();

    // Đặt các entry vào grid
    // gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_entry, 1, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_entry, 1, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_entry, 1, 4, 2, 1);

    // Tạo box chứa nút
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_grid_attach(GTK_GRID(grid), box, 0, 5, 3, 1);

    // Tạo các nút CANCEL và SAVE
    GtkWidget *cancel_button = createButton(box, "CANCEL");
    GtkWidget *save_button = createButton(box, "SAVE");

    // Hiển thị cửa sổ con
    gtk_widget_show_all(addCustomers_window);

    // Handle SAVE button
    AddCustomerData *add_data = g_new(AddCustomerData, 1);
    // add_data->id_entry = id_entry;
    add_data->name_entry = name_entry;
    add_data->phone_entry = numberphone_entry;
    add_data->plate_entry = numberplate_entry;
    add_data->type_entry = cartype_entry;
    add_data->store = data->store;

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), add_data);

    g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), addCustomers_window);

    g_signal_connect(addCustomers_window, "destroy", G_CALLBACK(free_memory_when_main_window_destroy), add_data);
}

void deleteCustomers(GtkWidget *widget, gpointer user_data)
{
    CustomerData *data = (CustomerData *)user_data;

    // Thiết lập cửa sổ
    GtkWidget *deleteCustomers_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(deleteCustomers_window), "Xóa khách hàng");
    gtk_window_set_transient_for(GTK_WINDOW(deleteCustomers_window), GTK_WINDOW(data->main_window));  // Gắn vào cửa sổ chính
    gtk_window_set_modal(GTK_WINDOW(deleteCustomers_window), TRUE);  // Làm cửa sổ phụ modal
    gtk_window_set_default_size(GTK_WINDOW(deleteCustomers_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(deleteCustomers_window), GTK_WIN_POS_CENTER);

    // Thiết lập các box trong cửa sổ
    GtkWidget *box_big = createBox(deleteCustomers_window, GTK_ORIENTATION_VERTICAL, 10); // Box lớn chứa 3 box con
    GtkWidget *box_entry = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thanh tìm kiếm
    GtkWidget *box_information = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thông tin của khách hàng tìm kiếm
    GtkWidget *box_delete = createBox(box_big, GTK_ORIENTATION_VERTICAL, 10); // Box chứa nút cancel và delete

    // Thiết lập cho box_entry
    GtkWidget *label = createLabel(box_entry, "Nhập vào mã KH:");
    GtkWidget *entry = createSearch(box_entry);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nhập chính xác mã khách hàng muốn xóa");
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 80);

    // Thiết lập cho box_information
    GtkWidget *grid = createGrid(box_information);

    GtkWidget *id_label = gtk_label_new("Mã KH:");
    GtkWidget *name_label = gtk_label_new("Tên KH:");
    GtkWidget *numberphone_label = gtk_label_new("SĐT:");
    GtkWidget *numberplate_label = gtk_label_new("Biển số:");
    GtkWidget *cartype_label = gtk_label_new("Loại xe:");

    gtk_grid_attach(GTK_GRID(grid), id_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_label, 0, 4, 1, 1);

    // Lấy thông tin từ Liststore để hiển thị
    FindIterOfSearch *findData = g_new0(FindIterOfSearch, 1);
    findData->list_store = data->store;
    findData->search_column = 0;
    findData->result_iter = g_new0(GtkTreeIter, 1);  // cấp phát cho iter
    findData->grid = grid;
    g_signal_connect(entry, "changed", G_CALLBACK(search_in_liststore_customer), findData);

    // Thiết lập cho box_delete
    GtkWidget *confirm_label = createLabel(box_delete, "Bạn có chắc chắn muốn xóa khách hàng này?");
    GtkWidget *confirm_box = createBox(box_delete, GTK_ORIENTATION_HORIZONTAL, 40);
    gtk_widget_set_halign(confirm_box, GTK_ALIGN_CENTER); // Căn giữa theo chiều ngang
    gtk_widget_set_valign(confirm_box, GTK_ALIGN_CENTER); // Căn giữa theo chiều dọc
    GtkWidget *cancel_button = createButton(confirm_box, "CANCEL");
    GtkWidget *delete_button = createButton(confirm_box, "DELETE");

    // Hiển thị cửa sổ con
    gtk_widget_show_all(deleteCustomers_window);

    // Handle DELETE button
    g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_button_clicked), findData);

    // Handle CANCEL button
    g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), deleteCustomers_window);

    // Giải phóng FindIterOfSearch
    g_signal_connect(deleteCustomers_window, "destroy", G_CALLBACK(free_struct_and_iter_customer), findData);
}

void editCustomers(GtkWidget *widget, gpointer user_data)
{
    CustomerData *data = (CustomerData *)user_data;

    // Thiết lập cửa sổ
    GtkWidget *editCustomers_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(editCustomers_window), "Sửa khách hàng");
    gtk_window_set_transient_for(GTK_WINDOW(editCustomers_window), GTK_WINDOW(data->main_window));  // Gắn vào cửa sổ chính
    gtk_window_set_modal(GTK_WINDOW(editCustomers_window), TRUE);  // Làm cửa sổ phụ modal
    gtk_window_set_default_size(GTK_WINDOW(editCustomers_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(editCustomers_window), GTK_WIN_POS_CENTER);

    // Thiết lập các box trong cửa sổ
    GtkWidget *box_big = createBox(editCustomers_window, GTK_ORIENTATION_VERTICAL, 10); // Box lớn chứa 3 box con
    GtkWidget *box_entry = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thanh tìm kiếm
    GtkWidget *box_information = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thông tin của khách hàng tìm kiếm
    GtkWidget *box_edit = createBox(box_big, GTK_ORIENTATION_VERTICAL, 10); // Box chứa nút cancel và edit

    // Thiết lập cho box_entry
    GtkWidget *label = createLabel(box_entry, "Nhập vào mã KH:");
    GtkWidget *entry = createSearch(box_entry);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nhập chính xác mã khách hàng muốn chỉnh sửa");
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 80);

    // Thiết lập cho box_information
    GtkWidget *grid = createGrid(box_information);

    // GtkWidget *id_label = gtk_label_new("Mã KH:");
    GtkWidget *name_label = gtk_label_new("Tên KH:");
    GtkWidget *numberphone_label = gtk_label_new("SĐT:");
    GtkWidget *numberplate_label = gtk_label_new("Biển số:");
    GtkWidget *cartype_label = gtk_label_new("Loại xe:");

    // gtk_grid_attach(GTK_GRID(grid), id_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_label, 0, 4, 1, 1);

    // GtkWidget *id_entry =  gtk_search_entry_new();
    GtkWidget *name_entry =  gtk_search_entry_new();
    GtkWidget *numberphone_entry =  gtk_search_entry_new();
    GtkWidget *numberplate_entry =  gtk_search_entry_new();
    GtkWidget *cartype_entry =  gtk_search_entry_new();

    // gtk_grid_attach(GTK_GRID(grid), id_entry, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_entry, 1, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_entry, 1, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_entry, 1, 4, 2, 1);
    // gtk_widget_set_hexpand(id_entry, TRUE);
    // gtk_widget_set_halign(id_entry, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(name_entry, TRUE);
    gtk_widget_set_halign(name_entry, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(numberphone_entry, TRUE);
    gtk_widget_set_halign(numberphone_entry, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(numberplate_entry, TRUE);
    gtk_widget_set_halign(numberplate_entry, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(cartype_entry, TRUE);
    gtk_widget_set_halign(cartype_entry, GTK_ALIGN_FILL);

    // Xử lí lấy thông tin từ Liststore để hiển thị
    FindIterOfSearch *findData = g_new0(FindIterOfSearch, 1);
    findData->list_store = data->store;
    findData->search_column = 0;
    findData->result_iter = g_new0(GtkTreeIter, 1);  // cấp phát cho iter
    findData->grid = grid;
    // findData->id_entry = id_entry;
    findData->name_entry = name_entry;
    findData->numberphone_entry = numberphone_entry;
    findData->numberplate_entry = numberplate_entry;
    findData->cartype_entry = cartype_entry;
    g_signal_connect(entry, "changed", G_CALLBACK(search_in_liststore_customer_edit), findData);

    // Thiết lập cho box_edit
    GtkWidget *confirm_label = createLabel(box_edit, "Bạn có chắc chắn muốn chỉnh sửa khách hàng này?");
    GtkWidget *confirm_box = createBox(box_edit, GTK_ORIENTATION_HORIZONTAL, 40);
    gtk_widget_set_halign(confirm_box, GTK_ALIGN_CENTER); // Căn giữa theo chiều ngang
    gtk_widget_set_valign(confirm_box, GTK_ALIGN_CENTER); // Căn giữa theo chiều dọc
    GtkWidget *cancel_button = createButton(confirm_box, "CANCEL");
    GtkWidget *edit_button = createButton(confirm_box, "EDIT");

    // Hiển thị cửa sổ con
    gtk_widget_show_all(editCustomers_window);

    // Handle EDIT button
    g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_button_clicked), findData);

    // Handle CANCEL button
    g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), editCustomers_window);

    // Giải phóng FindIterOfSearch
    g_signal_connect(editCustomers_window, "destroy", G_CALLBACK(free_struct_and_iter_customer), findData);
}

void historyCustomers(GtkWidget *widget, gpointer user_data)
{
    CustomerData *data = (CustomerData *)user_data;

    // Thiết lập cửa sổ
    GtkWidget *historyCustomers_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(historyCustomers_window), "Lịch sử khách hàng");
    gtk_window_set_transient_for(GTK_WINDOW(historyCustomers_window), GTK_WINDOW(data->main_window));  // Gắn vào cửa sổ chính
    gtk_window_set_modal(GTK_WINDOW(historyCustomers_window), TRUE);  // Làm cửa sổ phụ modal
    gtk_window_set_default_size(GTK_WINDOW(historyCustomers_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(historyCustomers_window), GTK_WIN_POS_CENTER);

    // Thiết lập các box trong cửa sổ
    GtkWidget *box_big = createBox(historyCustomers_window, GTK_ORIENTATION_VERTICAL, 10); // Box lớn chứa 3 box con
    GtkWidget *box_entry = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thanh tìm kiếm
    GtkWidget *box_information = createBox(box_big, GTK_ORIENTATION_HORIZONTAL, 10); // Box chứa thông tin của khách hàng tìm kiếm
    GtkWidget *box_back = createBox(box_big, GTK_ORIENTATION_VERTICAL, 10); // Box chứa nút Back

    // Thiết lập cho box_entry
    GtkWidget *label = createLabel(box_entry, "Nhập vào mã KH:");
    GtkWidget *entry = createSearch(box_entry);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nhập chính xác mã khách hàng muốn xem lịch sử");
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 80);

    // Thiết lập cho box_information
    // box_information gồm một grid bên trái hiện thông tin khách hàng và box bên phải hiện lịch sử

    // Grid thông tin
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(box_information), grid, TRUE, TRUE, 0);

    GtkWidget *id_label = gtk_label_new("Mã KH:");
    GtkWidget *name_label = gtk_label_new("Tên KH:");
    GtkWidget *numberphone_label = gtk_label_new("SĐT:");
    GtkWidget *numberplate_label = gtk_label_new("Biển số:");
    GtkWidget *cartype_label = gtk_label_new("Loại xe:");

    gtk_grid_attach(GTK_GRID(grid), id_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberphone_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), numberplate_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cartype_label, 0, 4, 1, 1);

    // Xử lí lấy thông tin từ Liststore để hiển thị
    FindIterOfSearch *findData = g_new0(FindIterOfSearch, 1);
    findData->list_store = data->store;
    findData->search_column = 0;
    findData->result_iter = g_new0(GtkTreeIter, 1);  // cấp phát cho iter
    findData->grid = grid;
    g_signal_connect(entry, "changed", G_CALLBACK(search_in_liststore_customer), findData);

    // Box lịch sử
    GtkWidget *box_history =gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(box_information), box_history, TRUE, TRUE, 0);

    GtkWidget *grid_history = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_history), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid_history), 10);
    gtk_widget_set_hexpand(grid_history, TRUE);   // Cho phép giãn ngang
    gtk_grid_set_column_homogeneous(GTK_GRID(grid_history), TRUE);

    GtkWidget *time_label = gtk_label_new("Thời gian");
    GtkWidget *service_label = gtk_label_new("Dịch vụ");
    GtkWidget *cost_label = gtk_label_new("Giá");
    gtk_widget_set_name(time_label, "title");
    gtk_widget_set_name(service_label, "title");
    gtk_widget_set_name(cost_label, "title");

    gtk_grid_attach(GTK_GRID(grid_history), time_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_history), service_label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_history), cost_label, 2, 0, 1, 1);

    // Tạo thanh cuộn cho grid_history ở trong box_history
    GtkWidget *scrolled = createScrolled(box_history, grid_history);

    // Xử lí lấy lịch sử từ Liststore của tab service và tab hóa đơn
    HistoryData *history_data = g_new(HistoryData, 1);
    history_data->billingList = data->billingList;
    history_data->serviceList = data->serviceList;
    history_data->grid_history = GTK_GRID(grid_history);
    history_data->entry = entry;
    g_signal_connect(entry, "changed", G_CALLBACK(search_customer_history), history_data);

    // Thiết lập box_back
    gtk_widget_set_halign(box_back, GTK_ALIGN_CENTER); // Căn giữa theo chiều ngang
    GtkWidget *back_button = createButton(box_back, "BACK");

    // Hiển thị cửa sổ con
    gtk_widget_show_all(historyCustomers_window);

    // Handle BACK button
    g_signal_connect_swapped(back_button, "clicked", G_CALLBACK(gtk_widget_destroy), historyCustomers_window);

    // Giải phóng bộ nhớ cho các struct khi tắt cửa sổ con
    g_signal_connect(historyCustomers_window, "destroy", G_CALLBACK(free_struct_and_iter_customer), findData);
    g_signal_connect(historyCustomers_window, "destroy", G_CALLBACK(free_memory_when_main_window_destroy), history_data);
}   

// Hàm xử lý khi nhấn nút "Gửi đánh giá"
 void on_rate_customer_search(GtkWidget *button, gpointer user_data) {
        GtkWidget *search_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "entry"));
        const gchar *search_id = gtk_entry_get_text(GTK_ENTRY(search_entry));
        CustomerData *data = (CustomerData *)g_object_get_data(G_OBJECT(button), "data");
    
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(data->store), &iter);
    
        while (valid) {
            gchar *id, *name, *phone, *plate, *type;
            gtk_tree_model_get(GTK_TREE_MODEL(data->store), &iter,
                               0, &id,
                               1, &name,
                               2, &phone,
                               3, &plate,
                               4, &type,
                               -1);
    
            if (g_strcmp0(id, search_id) == 0) {
                gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "id_label")), id);
                gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "name_label")), name);
                gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "phone_label")), phone);
                gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "plate_label")), plate);
                gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "type_label")), type);
    
                g_free(id);
                g_free(name);
                g_free(phone);
                g_free(plate);
                g_free(type);
                return;
            }
    
            g_free(id);
            g_free(name);
            g_free(phone);
            g_free(plate);
            g_free(type);
    
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(data->store), &iter);
        }
    
        // Nếu không tìm thấy
        showMessage("Không tìm thấy khách hàng!");}
    
    

void on_rate_customer_clicked(GtkWidget *widget, gpointer user_data) {
    CustomerData *data = (CustomerData *)user_data;

    // Tạo cửa sổ đánh giá
    GtkWidget *rate_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(rate_window), "Đánh giá dịch vụ");
    gtk_window_set_default_size(GTK_WINDOW(rate_window), 500, 400);
    gtk_window_set_transient_for(GTK_WINDOW(rate_window), GTK_WINDOW(data->main_window));
    gtk_window_set_modal(GTK_WINDOW(rate_window), TRUE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(rate_window), vbox);

    // Thanh tìm kiếm
    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Nhập mã KH...");
    GtkWidget *search_button = gtk_button_new_with_label("Tìm kiếm");
    gtk_box_pack_start(GTK_BOX(search_box), search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), search_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), search_box, FALSE, FALSE, 0);

    // Grid hiển thị thông tin khách hàng
    GtkWidget *info_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(info_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(info_grid), 10);
    gtk_box_pack_start(GTK_BOX(vbox), info_grid, FALSE, FALSE, 10);

    GtkWidget *id_label = gtk_label_new("");
    GtkWidget *name_label = gtk_label_new("");
    GtkWidget *phone_label = gtk_label_new("");
    GtkWidget *plate_label = gtk_label_new("");
    GtkWidget *type_label = gtk_label_new("");

    gtk_grid_attach(GTK_GRID(info_grid), gtk_label_new("Mã KH:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), id_label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), gtk_label_new("Tên KH:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), name_label, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), gtk_label_new("SĐT:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), phone_label, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), gtk_label_new("Biển số:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), plate_label, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), gtk_label_new("Loại xe:"), 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), type_label, 1, 4, 1, 1);

    // Thanh trượt đánh giá
    GtkWidget *rating_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 10, 1);
    gtk_scale_set_digits(GTK_SCALE(rating_scale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(rating_scale), GTK_POS_TOP);

    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Đánh giá (0-10):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), rating_scale, FALSE, FALSE, 0);

    // Ô nhập nhận xét
    GtkWidget *comment_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(comment_entry), "Nhập nhận xét...");
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("Nhận xét:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), comment_entry, FALSE, FALSE, 0);

    // Nút gửi đánh giá
    GtkWidget *submit_button = gtk_button_new_with_label("Gửi đánh giá");
    gtk_box_pack_start(GTK_BOX(vbox), submit_button, FALSE, FALSE, 10);

    // Tạo context để truyền dữ liệu cho callback
    RateContext *ctx = g_malloc(sizeof(RateContext));
    ctx->entry = search_entry;
    ctx->scale = rating_scale;           // dùng scale thay vì spin
    ctx->entry_cmt = comment_entry;
    ctx->data = data;
    ctx->window = rate_window;

    // Gắn các label vào button để callback lấy lại
    g_object_set_data(G_OBJECT(search_button), "id_label", id_label);
    g_object_set_data(G_OBJECT(search_button), "name_label", name_label);
    g_object_set_data(G_OBJECT(search_button), "phone_label", phone_label);
    g_object_set_data(G_OBJECT(search_button), "plate_label", plate_label);
    g_object_set_data(G_OBJECT(search_button), "type_label", type_label);
    g_object_set_data(G_OBJECT(search_button), "entry", search_entry);
    g_object_set_data(G_OBJECT(search_button), "data", data);

    // Gán callback
    g_signal_connect(search_button, "clicked", G_CALLBACK(on_rate_customer_search), NULL);
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_rate_submit), ctx);

    gtk_widget_show_all(rate_window);
}

void on_rate_submit(GtkWidget *button, gpointer user_data) {
    RateContext *ctx = (RateContext *)user_data;

    const gchar *id = gtk_entry_get_text(GTK_ENTRY(ctx->entry));
    gint rating = (gint)gtk_range_get_value(GTK_RANGE(ctx->scale));
    const gchar *comment = gtk_entry_get_text(GTK_ENTRY(ctx->entry_cmt));

    if (id == NULL || g_strcmp0(id, "") == 0) {
        showMessage("Vui lòng nhập mã khách hàng!");
        return;
    }

    if (g_strcmp0(comment, "") == 0) {
        showMessage("Vui lòng nhập nhận xét!");
        return;
    }

    // TODO: Ghi thông tin đánh giá vào cơ sở dữ liệu hoặc file
    g_print("Mã KH: %s\n", id);
    g_print("Điểm: %d\n", rating);
    g_print("Nhận xét: %s\n", comment);

    showMessage("Đánh giá đã được gửi. Cảm ơn bạn!");

    gtk_widget_destroy(ctx->window);
    g_free(ctx);
}

void showMessage(const gchar *message) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_INFO,
                                                GTK_BUTTONS_OK,
                                                "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Thông báo");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

