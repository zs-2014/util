#coding: utf-8
import gevent
import os, sys
import signal
import errno
import traceback
import logging
import multiprocessing
import functools
import itertools
import copy

import config
import util
import notify_handler

from util import get_errno_from_exception

log = logging.getLogger()

signal_map = dict([(getattr(signal, sig), sig) if sig.startswith('SIG') else (-1, 'UNKNOWN') for sig in dir(signal)])

class Workers(object):
    def __init__(self, notify_server, queue):
        self.notify_server = notify_server
        self.server_child_pids = []
        self.queue = queue
        self._notify = None

    def run(self):
        self.server_works = config.works if config.works is not None else multiprocessing.cpu_count()
        self.notify_threads = config.notify_threads

        if self.server_works <= 0 or self.notify_threads <= 0:
            raise ValueError('notify_works %s or notify_threads %s is not valid' % (notify_works, notify_threads))
        self.spawn_server_workers()
        self.start_monitor()

    def spawn_server_workers(self):
        is_load_from_store = True
        for _ in range(0, self.server_works):
            pid = os.fork() 
            #child process
            if pid == 0:
                try:
                    self.install_child_process_signal_handler()
                    self._notify = notify_handler.Notify(self.queue, self.notify_threads, is_load_from_store)
                    gevent.spawn(self._notify.do_notify)
                    self.notify_server.run(1)
                finally: 
                    self.notify_server.stop()
                    sys.exit(0) 
            else:
                log.info('child process %d is starting', pid)
                is_load_from_store = False
                self.server_child_pids.append(pid) 
        return True

    def start_monitor(self):
        log.info('master process %d is starting', os.getpid())
        self.install_parent_process_signal_handler()
        while self.server_child_pids:
            try:
                pid, status = os.wait()
            except Exception, e:
                if get_errno_from_exception(e) == errno.EINTR:
                    continue 
                log.warn(traceback.format_exc())
                continue
            if pid not in self.server_child_pids:
                continue
            self.server_child_pids.remove(pid)
            if os.WIFSIGNALED(status): 
                signo = os.WTERMSIG(status)
                log.info('child process %d was killed by signal %s(%d) restarting ...', pid, signal_map.get(signo), signo)
            elif os.WEXITSTATUS(status) != 0:
                log.info('child process %d exited with status %d restarting ...', pid, os.WEXITSTATUS(status))
            else:
                log.info('child process %d exited normally', pid)
                continue
            self.spawn_server_workers() 
        log.info('master process %d is terminated', os.getpid())
        sys.exit(0)

    def log_signal(self, signo, tb):
        log.debug('process %s receive a signal %s(%d)', os.getpid(), signal_map.get(signo), signo) 

    def handle_sig_usr(self, signo, tb):
        self.notify_server.stop()
        if self._notify:
            self._notify.stop()

    def handle_term_signal(self, signo, tb):
        child_pids = [i for i in self.server_child_pids]
        for pid in child_pids:
            log.info('kill child process %d', pid)
            os.kill(pid, signal.SIGUSR1)

    def handle_int_signal(self, signo, tb):
        self.handle_term_signal(signo, tb)

    def install_parent_process_signal_handler(self):
        ign_signo = [signal.SIGINT, signal.SIGCHLD, signal.SIGPIPE, signal.SIGHUP, signal.SIGQUIT]
        for s in ign_signo:
            signal.signal(s, self.log_signal)
        signal.signal(signal.SIGTERM, self.handle_term_signal)
        signal.signal(signal.SIGINT, self.handle_int_signal)
        
    def install_child_process_signal_handler(self):
        signo = [signal.SIGINT, signal.SIGPIPE, signal.SIGHUP, signal.SIGTERM, signal.SIGQUIT] 
        for s in signo:
            signal.signal(s, self.log_signal)
        signal.signal(signal.SIGUSR1, self.handle_sig_usr)
        signal.signal(signal.SIGUSR2, self.handle_sig_usr)
