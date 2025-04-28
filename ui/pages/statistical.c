#include "statistical.h"
#include "../components/box.h"
#include "../components/button.h"
#include "../components/treeView.h"
#include "../components/search.h"
#include "../components/page.h"
#include "../utils/listStore.h"
#include "../../model/model_central.h"

gdouble get_service_price(GtkListStore *serviceList, const gchar *service_id) {
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(serviceList), &iter);

    while (valid) {
        gchar *id;
        gchar *price_str;
        gdouble price = 0.0;

        gtk_tree_model_get(GTK_TREE_MODEL(serviceList), &iter,
                           0, &id,        // mã dịch vụ
                           2, &price_str, // giá tiền (dạng chuỗi)
                           -1);

        if (g_strcmp0(id, service_id) == 0) {
            if (price_str != NULL)
            {
                price = g_ascii_strtod(price_str, NULL);
            }

            g_free(id);
            g_free(price_str);
            return price;
        }

        g_free(id);
        g_free(price_str);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(serviceList), &iter);
    }

    return 0.0;
}

void populate_stats_tree_store(GtkTreeStore *store, GtkListStore *billingList, GtkListStore *serviceList) {
    GHashTable *year_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GHashTable *month_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GHashTable *day_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(billingList), &iter);

    while (valid) {
        gchar *date_str, *service_id;
        gtk_tree_model_get(GTK_TREE_MODEL(billingList), &iter,
                           1, &date_str,  // ngày
                           3, &service_id, // mã dịch vụ
                           -1);

                        // Xóa khoảng trắng thừa trong service_id
                        g_strstrip(service_id);

        gint day, month, year;
        sscanf(date_str, "%d-%d-%d", &day, &month, &year);

        // Lấy giá dịch vụ từ serviceList
        gdouble price = get_service_price(serviceList, service_id);

        gchar *key_year = g_strdup_printf("%d", year);
        gchar *key_month = g_strdup_printf("%d-%02d", year, month);
        gchar *key_day = g_strdup_printf("%d-%02d-%02d", year, month, day);

        GtkTreeIter *year_iter = g_hash_table_lookup(year_hash, key_year);
        if (!year_iter) {
            year_iter = g_new(GtkTreeIter, 1);
            gtk_tree_store_append(store, year_iter, NULL);
            gtk_tree_store_set(store, year_iter,
                               0, key_year,
                               1, 1,
                               2, price,
                               -1);
            g_hash_table_insert(year_hash, key_year, year_iter);
        } else {
            gint car_count;
            gdouble revenue;
            gtk_tree_model_get(GTK_TREE_MODEL(store), year_iter, 1, &car_count, 2, &revenue, -1);
            gtk_tree_store_set(store, year_iter,
                               1, car_count + 1,
                               2, revenue + price,
                               -1);
            g_free(key_year);
        }

        GtkTreeIter *month_iter = g_hash_table_lookup(month_hash, key_month);
        if (!month_iter) {
            month_iter = g_new(GtkTreeIter, 1);
            gtk_tree_store_append(store, month_iter, year_iter);
            gchar *month_label = g_strdup_printf("Tháng %d", month);
            gtk_tree_store_set(store, month_iter,
                               0, month_label,
                               1, 1,
                               2, price,
                               -1);
            g_hash_table_insert(month_hash, key_month, month_iter);
            g_free(month_label);
        } else {
            gint car_count;
            gdouble revenue;
            gtk_tree_model_get(GTK_TREE_MODEL(store), month_iter, 1, &car_count, 2, &revenue, -1);
            gtk_tree_store_set(store, month_iter,
                               1, car_count + 1,
                               2, revenue + price,
                               -1);
            g_free(key_month);
        }

        GtkTreeIter *day_iter = g_hash_table_lookup(day_hash, key_day);
        if (!day_iter) {
            day_iter = g_new(GtkTreeIter, 1);
            gtk_tree_store_append(store, day_iter, month_iter);
            gchar *day_label = g_strdup_printf("Ngày %d", day);
            gtk_tree_store_set(store, day_iter,
                               0, day_label,
                               1, 1,
                               2, price,
                               -1);
            g_hash_table_insert(day_hash, key_day, day_iter);
            g_free(day_label);
        } else {
            gint car_count;
            gdouble revenue;
            gtk_tree_model_get(GTK_TREE_MODEL(store), day_iter, 1, &car_count, 2, &revenue, -1);
            gtk_tree_store_set(store, day_iter,
                               1, car_count + 1,
                               2, revenue + price,
                               -1);
            g_free(key_day);
        }

        g_free(date_str);
        g_free(service_id);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(billingList), &iter);
    }

    g_hash_table_destroy(year_hash);
    g_hash_table_destroy(month_hash);
    g_hash_table_destroy(day_hash);
}

static gboolean draw_column_chart(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkTreeStore *store = (GtkTreeStore *)user_data;
    int width, height;
    gtk_widget_get_size_request(widget, &width, &height);

    int padding = 70;    // Khoảng cách biên
    int bar_width = 60;  // Chiều rộng cột
    int spacing = 30;    // Khoảng cách giữa các cột

    // Tính doanh thu cao nhất
    gdouble max_revenue = 0;
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    while (valid) {
        gchar *time_label;
        gint car_count;
        gdouble revenue;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &time_label, 1, &car_count, 2, &revenue, -1);
        if (revenue > max_revenue) {
            max_revenue = revenue;
        }
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }

    if (max_revenue == 0) max_revenue = 1; // Tránh chia cho 0

  // Vẽ tiêu đề
cairo_set_source_rgb(cr, 0, 0, 0); // Chọn màu vẽ là màu đen (RGB: 0, 0, 0)
cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); // Chọn kiểu font là Arial, không in nghiêng, đậm
cairo_set_font_size(cr, 21); // Thiết lập kích thước font là 20
cairo_move_to(cr, width / 2 - 150, padding - 30); // Di chuyển điểm bắt đầu vẽ văn bản tới vị trí (width / 2 - 150, padding - 30)
cairo_show_text(cr, "BIỂU ĐỒ MIÊU TẢ DOANH THU THEO NĂM"); // Vẽ văn bản "BIỂU ĐỒ MIÊU TẢ DOANH THU THEO NĂM"


    // Vẽ trục tung
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, padding, padding - 34);
    cairo_line_to(cr, padding, height - padding);
    cairo_stroke(cr);

    // Chia trục tung mỗi 500k
    gdouble tick_value = 500000.0; // 500k
    int num_ticks = (int)(max_revenue / tick_value) + 1;

    cairo_set_font_size(cr, 12); // Cỡ chữ nhỏ cho trục tung

    for (int i = 0; i <= num_ticks; i++) {
        gdouble value = i * tick_value;
        int y_pos = height - padding - (int)((value / max_revenue) * (height - 2 * padding));

        // Vẽ vạch nhỏ
        cairo_move_to(cr, padding - 5, y_pos);
        cairo_line_to(cr, padding + 5, y_pos);
        cairo_stroke(cr);

        // Tạo nhãn theo dạng "500k", "1M", "1.5M", "2M", ...
        gchar *label = NULL;
        if (value < 1000000) {
            // Dưới 1 triệu: hiển thị 500k, 1M, ...
            int thousands = (int)(value / 1000);
            label = g_strdup_printf("%dk", thousands);
        } else {
            // Từ 1 triệu trở lên: hiển thị 1M, 1.5M, ...
            gdouble millions = value / 1000000.0;
            label = g_strdup_printf("%.1fM", millions);
        }

        cairo_move_to(cr, padding - 45, y_pos + 5);
        cairo_show_text(cr, label);
        g_free(label);
    }

    // Vẽ các cột
    int x_offset = padding + 20; // Dời ra phải chút cho đẹp
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    while (valid) {
        gchar *time_label;
        gint car_count;
        gdouble revenue;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &time_label, 1, &car_count, 2, &revenue, -1);

        // Tính chiều cao cột
        int bar_height = (int)(revenue / max_revenue * (height - 2 * padding));

        // Vẽ cột
        cairo_set_source_rgb(cr, 0.2, 0.6, 0.2); // Xanh lá
        cairo_rectangle(cr, x_offset, height - padding - bar_height, bar_width, bar_height);
        cairo_fill(cr);

                // Đặt font và kích thước chữ
        cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); // Chọn font Arial, kiểu chữ thường, đậm
        cairo_set_font_size(cr, 15);  // Thay đổi kích thước font

        cairo_set_source_rgb(cr, 0, 0, 0);  // Màu đen cho chữ

        // Di chuyển và ghi nhãn thời gian dưới cột
        cairo_move_to(cr, x_offset + (bar_width / 2) - 15, height - padding + 30); // Di chuyển nhãn thấp hơn một chút
        cairo_show_text(cr, time_label);

        // Cập nhật x_offset để tiếp tục vẽ các cột tiếp theo
        x_offset += bar_width + spacing;

        // Di chuyển đến phần tử tiếp theo trong TreeStore
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);

            }

    return FALSE;
}


GtkWidget *createStatisticalPage(GtkWidget *notebook, GtkWidget *window, gpointer user_data) {
    GtkWidget *page = createPage(notebook, GTK_ORIENTATION_VERTICAL, 10, "Thống kê & Báo cáo");

    GtkTreeStore *store = gtk_tree_store_new(3,
                                             G_TYPE_STRING,   // Cột 0: Thời gian
                                             G_TYPE_INT,      // Cột 1: Số xe
                                             G_TYPE_DOUBLE);  // Cột 2: Doanh thu

    modelCentral *data = (modelCentral *)user_data;
    populate_stats_tree_store(store, data->billingList, data->serviceList);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(page), scrolled_window);

    GtkWidget *statsTreeView = createTreeView(scrolled_window);
    const gchar *columnNames[] = {"Thời gian", "Số xe sửa chữa", "Doanh thu (VND)"};
    createColumns(statsTreeView, columnNames, 3);
    gtk_tree_view_set_model(GTK_TREE_VIEW(statsTreeView), GTK_TREE_MODEL(store));

    // Tạo GtkDrawingArea cho biểu đồ cột
    GtkWidget *chart_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(chart_area, 600, 400); // Phóng to biểu đồ
    gtk_container_add(GTK_CONTAINER(page), chart_area);

    // Kết nối tín hiệu vẽ cho biểu đồ cột
    g_signal_connect(G_OBJECT(chart_area), "draw", G_CALLBACK(draw_column_chart), store);

    // Căn giữa biểu đồ trong page
    gtk_widget_set_halign(chart_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(chart_area, GTK_ALIGN_CENTER);

    return page;
}


