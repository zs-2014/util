#namespace py notifier

service Notifier
{
    string ping(),
    string send_notify(1:string notify_data)
}
