#include <portable-file-dialogs.h>
#include <stdexcept>

#include "file_picker.h"

FilePicker::FilePicker() : isShutdown_(false)
{
    thread_ = std::thread(&FilePicker::workerLoop, this); // initializes the file save handler in a seperate thread
}

FilePicker::~FilePicker()
{
    {
        std::lock_guard lock(mutex_);
        isShutdown_ = true;
    }
    cv_.notify_one();

    if (thread_.joinable())
        thread_.join();
}

static std::string makeFilterLabel(const std::string &ext)
{
    std::string label;
    label.reserve(ext.size() + 6); // ext + " Files"

    for (char c : ext)
        label += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    return label + " Files";
}

std::string FilePicker::ensureExtension(std::string path, const std::string &ext)
{
    const std::string dotExt = "." + ext;
    if (path.size() < dotExt.size() || path.compare(path.size() - dotExt.size(), dotExt.size(), dotExt) != 0)
        path += dotExt;

    return path;
}

std::string FilePicker::query(const Entry &entry, bool isSave, std::function<void(const std::string &)> onResult)
{
    std::future<std::string> future;
    {
        std::lock_guard lock(mutex_);
        if (entries_.size() >= kMaxQueue)
            return "";

        QueuedEntry qe;
        qe.entry = entry;
        qe.isSave = isSave;
        qe.callback = onResult;
        
        future = qe.promise.get_future();
        entries_.push(std::move(qe));

        cv_.notify_one();
        if (onResult)
            return "";
        return future.get();
    }
}

void FilePicker::workerLoop()
{
    std::unique_lock<std::mutex> lock(mutex_); // allows for manual locking/unlocking
    while (!isShutdown_ || !entries_.empty())
    {
        cv_.wait(lock, [this]
                 { return isShutdown_ || !entries_.empty(); });

        if (entries_.empty())
            continue;

        QueuedEntry qe = std::move(entries_.front());
        entries_.pop();

        lock.unlock();
        std::string result = runDialog(qe);

        if (qe.callback)
            qe.callback(result);

        qe.promise.set_value(std::move(result));
        lock.lock();
    }
}

std::string FilePicker::runDialog(const QueuedEntry &qe)
{
    const std::string title = qe.entry.dialogTitle.empty() ? qe.isSave ? "Save File" : "Open File" : qe.entry.dialogTitle;

    // pairs of {"Lablel", "*.ext"}
    const std::string label = makeFilterLabel(qe.entry.fileExtension);
    const std::string glob = "*." + qe.entry.fileExtension;

    if (qe.isSave)
    {
        const std::string defaultName = qe.entry.defaultFilename.empty() ? ("file." + qe.entry.fileExtension) : qe.entry.defaultFilename;

        std::string destination = pfd::save_file(title, defaultName, {label, glob}).result();
        if (destination.empty())
            return {};

        return ensureExtension(std::move(destination), qe.entry.fileExtension);
    }
    else
    {
        pfd::open_file dialog = pfd::open_file(title, "", {label, glob});
        std::vector<std::string> files = dialog.result();

        return files.empty() ? std::string{} : files.front();
    }
}
